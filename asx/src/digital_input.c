/**
 * @file
 * [Timer](group__timer.html) service implementation
 * @internal
 * @addtogroup service
 * @{
 * @addtogroup digital_input
 * @{
 * @author gax
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#include "alert.h"
#include "digital_input.h"
#include "timer.h"
#include "mem.h"


/************************************************************************/
/* Define(s)                                                            */
/************************************************************************/

/** 
 * @def DIGITAL_INPUT_PRIO
 * Assign a priority to the digital input reactor handler
 * Defaults to reactor_prio_medium_plus
 */
#ifndef DIGITAL_INPUT_PRIO
#  define DIGITAL_INPUT_PRIO reactor_prio_medium_plus
#endif

/** 
 * @def DIGITAL_INPUT_ACK_PRIO
 * Assign a priority to the digital input acknowledgment handler
 * Defaults to reactor_prio_very_high_minus
 */
#ifndef DIGITAL_INPUT_ACK_PRIO
#  define DIGITAL_INPUT_ACK_PRIO reactor_prio_very_high_minus
#endif

/** 
 * @def DIGITAL_INPUT_SAMPLE_PERIOD
 * Common sampling period.
 * Defaults to 5ms
 */
#ifndef DIGITAL_INPUT_SAMPLE_PERIOD
#  define DIGITAL_INPUT_SAMPLE_PERIOD TIMER_MILLISECONDS(5)
#endif


/************************************************************************/
/* Local variables                                                      */
/************************************************************************/

/** First structures to receive the di-> Other are chained */
static digital_input_t *_first_sampled = {0};

/** First structures to receive the di-> Other are chained */
static digital_input_t *_first_direct = {0};

/** Reactor for managing the sampling of the inputs */
static reactor_handle_t _react_sample;

/** Reactor for acknowledging the interrupts */
static reactor_handle_t _react_direct_handler;

/** Reactor for acknowledging the interrupts */
static reactor_handle_t _react_ack_it;

/** Mask of bits which are being processed by not acknowledged */
static volatile uint8_t _isr_bit_mask[2] = {0,0};


/************************************************************************/
/* Private functions                                                    */
/************************************************************************/

/** 
 * Called by the timer at regular interval to sample the digital inputs
 */
static void _digital_input_sample(void *arg)
{
   // Grab first input
   digital_input_t *di = _first_sampled;

   // Iterate over each input
   while ( di )
   {
      bool level = ioport_get_pin_level(di->pin);
      bool previous_input = di->sampled.input;
      
      if ( level )
      {
         if ( di->sampled.integrator < di->sampled.integrator_threshold )
         {
            ++di->sampled.integrator;
            
            if ( di->sampled.integrator == di->sampled.integrator_threshold )
            {
               di->sampled.input = true;
            }
         }
      }
      else
      {
         if ( di->sampled.integrator )
         {
            --di->sampled.integrator;

            if ( di->sampled.integrator == 0 )
            {
               di->sampled.input = false;
            }
         }
      }
      
      // Check the integrator result and invoke handler on change
      if ( di->sampled.input != previous_input && di->handler != REACTOR_NULL_HANDLE )
      {
         pin_and_value_t pav = {.pin=di->pin};
         pav.value = di->sampled.input;
         
         reactor_notify(di->handler, pav.as_arg);
      }
      
      // Move to the next
      di = di->next;
   };
}

/** 
 * Clear an interrupt after it triggered
 * Can be called from a reactor or directly
 * @param arg A pin_and_value_t argument. The value is ignored.
 */
static void _clear_interrupt(void *arg)
{
   digital_input_t *next = (digital_input_t *)arg;
   
   // Atomically handle the _isr_bit_mask and the interrupt flags

   /************************************************************************/
   /* Enter critical section                                               */
   /************************************************************************/
   sei();
   
   // Reset the bit to allow for processing interrupts again
   _isr_bit_mask[ioport_pin_to_port_id(next->pin)] &= (~ioport_pin_to_mask(next->pin));
  
   // Enable the interrupt again
   ioport_set_pin_sense_mode(next->pin, next->direct.sense_mode);
   
   /************************************************************************/
   /* Leave critical section                                               */
   /************************************************************************/
   cli();
}

/** 
 * Called by the reactor to handle a input change triggered by an interrupt
 * Notify the handler
 * Clears the interrupt to make way for new ones directly, or by arming a timer.
 */
static void _digital_input_direct_handler(void *arg)
{
   pin_and_value_t pav = (pin_and_value_t)arg;
   
   // Locate the digital_input responsible
   digital_input_t *next = _first_direct;
   
   while ( next )
   {
      if ( next->pin == pav.pin )
      {
         // Found the handler responsible
         if ( next->handler != REACTOR_NULL_HANDLE )
         {
            reactor_notify(next->handler, pav.as_arg);
         }
         
         // Acknowledge the ISR
         if ( next->direct.filter )
         {
            timer_arm(
               _react_ack_it, 
               timer_get_count_from_now(next->direct.filter),
               0, 
               (void *)next);
         }
         else
         {
            _clear_interrupt( (void *)next);
         }
         
         
         break;
      }         
         
      next = next->next;
   }
}


/** 
 * Common handler for both interrupts
 * Notify the handler, and arm a delayed interrupt acknowledgment if
 *  filtering is in place
 * @param index The port index. 0 for PORTA and 1 for PORTB
 * @param port_value The value of the port read right away
 * @param mask The interrupt mask telling which bits caused the ISR
 */
static void _handle_pin_change_isr(
   uint8_t port_id, uint8_t port_value, uint8_t mask)
{
   // Check the bit(s) and notify
   uint8_t i=0;
   
   // Get the current handling status
   uint8_t handling_mask = _isr_bit_mask[port_id];
   
   // Mask bits already being processed
   mask &= ~handling_mask;
   
   // Append new detected bits to avoid re-processing them
   _isr_bit_mask[port_id] |= mask;
   
   while (mask)
   {
      // If the interrupt flag is set but not yet set in the handling mask
      if ( mask & 1 )
      {
         // Notify the reactor to handle the change
         pin_and_value_t pav;
         pav.pin = ioport_create_pin(port_id, i);
         pav.value = port_value;
         
         // Turn interrupts off until acknowledge is called
         ioport_enable_pin(pav.pin);
         
         // Handle in the reactor - not in the interrupt
         reactor_notify( _react_direct_handler, pav.as_arg);
      }
      
      // Bit shift the mask and increment position
      mask >>= 1;
      ++i;
   }
}


/************************************************************************/
/* Public API                                                           */
/************************************************************************/

/** 
 * Create a digital input object 
 * @param p The port_io pin to watch
 * @param reactor A reactor handle which process any change. It can be a null handler.
 * @param sense_mode If IOPORT_SENSE_DISABLE, the input is sampled, otherwise the value determine what
 *         input change cause the handler to respond.
 * @param filter_value Number of count to wait count when filtering, or number of ms to wait
 *         before acknowledging the interrupt. For direct sensing (interrupt mode), a value
 *        of 0 means the interrupt is acknowledged immediately.
 *        Note: This could lead to a DoS if the input is not properly managed
 * @return A digital input handler. This can be used to read the value directly
 */
digital_input_handle_t digital_input(
   ioport_pin_t pin, reactor_handle_t reactor, uint8_t sense_mode, timer_count_t filter_value)
{
   // Pointer to the next pointer
   digital_input_t **next;
   
   // Allocate a new structure
   digital_input_t *di = mem_calloc(1, sizeof(digital_input_t));

   // Fill the common structure
   di->pin = pin;
   di->handler = reactor;
   
   if ( sense_mode != IOPORT_SENSE_DISABLE )
   {
      // Direct
      di->direct.sense_mode = sense_mode;
      di->direct.filter = filter_value;

      // Set the sense detection - enabling the interrupt detection
      ioport_set_pin_sense_mode(di->pin, sense_mode);

      next = &_first_direct;
   }
   else
   {
      // Regular
      di->sampled.integrator_threshold = filter_value / DIGITAL_INPUT_SAMPLE_PERIOD;
      
      next = &_first_sampled;
   }

   // Chain the structure
   while ( *next != NULL )
   {
      next = &((*next)->next);
   }
   
   *next = di;
   
   return (digital_input_handle_t)di;
}

/** 
 * Initialize the digital di->
 * Make sure that all input are registered before calling this function
 * For C++, the inputs can be registered as global variables.
 * For C/C++, the board_init must register the inputs first, then call this function.
 * Register the 2 reactors for direct and regular inputs
 */
void digital_input_init(void)
{
   // Register the react
   _react_sample = reactor_register(
      _digital_input_sample, DIGITAL_INPUT_PRIO, 1);

   _react_direct_handler = reactor_register(
      _digital_input_direct_handler, DIGITAL_INPUT_ACK_PRIO, 1);

   _react_ack_it = reactor_register(
      _clear_interrupt, DIGITAL_INPUT_ACK_PRIO, 1);

   // Start a repeating timer to sample the inputs at regular interval
   timer_arm(_react_sample, timer_get_count_from_now(0), DIGITAL_INPUT_SAMPLE_PERIOD, NULL);
}

/**
 * Return the current status of a sampled pin.
 * Only for sampled pins.
 */
bool digital_input_value(digital_input_handle_t di)
{
   return di->sampled.input;
}

/************************************************************************/
/* ISRs                                                                 */
/************************************************************************/
ISR(PORTA_PORT_vect)
{
   _handle_pin_change_isr(IOPORT_PORTA, PORTA.IN, PORTA.INTFLAGS);

   // Clear the interrupt
   PORTA.INTFLAGS |= PORTA.INTFLAGS;
}   

ISR(PORTB_PORT_vect)
{
   _handle_pin_change_isr(IOPORT_PORTB, PORTB.IN, PORTB.INTFLAGS);

   // Clear the interrupt
   PORTB.INTFLAGS |= PORTB.INTFLAGS;
}

/**@}*/
/**@}*/
/**@} ---------------------------  End of file  --------------------------- */