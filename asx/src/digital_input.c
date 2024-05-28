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
static digital_input_t *_first = {0};

/** Reactor for managing the sampling of the inputs */
static reactor_handle_t _react_sample;

/** Reactor for acknowledging the interrupts */
static reactor_handle_t _react_ack_it;

/** Mask of bits which have been processed by not acknowledged */
static volatile uint8_t isr_bit_to_acknowledge_mask[2] = {0,0};


/************************************************************************/
/* Private functions                                                    */
/************************************************************************/

/** 
 * Called by the timer at regular interval to sample the digital inputs
 */
static void _digital_input_sample(void *arg)
{
   // Grab first input
   digital_input_t *di = _first;

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
 * Re-enable the interrupt following a handler
 * Allow the application to filter direct handlers
 * This must be called after the reactor handler or the detection stops.
 */
static void _digital_input_ack(void *arg)
{
   pin_and_value_t pin_and_value = (pin_and_value_t)arg;
   ioport_pin_t pin = pin_and_value.pin;
   
   PORT_t *base = ioport_pin_to_base(pin);
   base->INTFLAGS |= ioport_pin_to_mask(pin);

   // Reset the bit
   isr_bit_to_acknowledge_mask[ioport_pin_to_port_id(pin)] &= (~ioport_pin_to_mask(pin));
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
   // Grab first input
   digital_input_t *di = _first;

   // Filter all bit which are yet to be acknowledged (to avoid DoS through noise)
   mask &= (~isr_bit_to_acknowledge_mask[port_id]);

   while ( di )
   {
      // Does the interrupt match
      if ( 
         (ioport_pin_to_port_id(di->pin) == port_id) &&
         (ioport_pin_to_mask(di->pin) & mask)
      ) {
         pin_and_value_t pav = {
            .pin = di->pin,
            .value = ioport_pin_to_mask(di->pin) & port_value
         };
         
         if ( di->handler != REACTOR_NULL_HANDLE )
         {
            reactor_notify(di->handler, pav.as_arg);
         }
      
         // Is filtering required?
         if ( di->direct.filter )
         {
            isr_bit_to_acknowledge_mask[port_id] &= ioport_pin_to_mask(di->pin);

            timer_arm(_react_ack_it, di->direct.filter, 0, pav.as_arg);
         }
         else
         {
            // Acknowledge here and now
            _digital_input_ack(pav.as_arg);
         }
      }

      di = di->next;
   }
}   

/************************************************************************/
/* Public API                                                           */
/************************************************************************/

/** 
 * Create a digital input object 
 * @param p The port_io pin to watch
 * @param reactor A reactor handle which process any change. It can be a null handler.
 * @param sense_mode If 0, the input is sampled, otherwise the value determine what
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
   // Allocate a new structure
   digital_input_t *di = mem_calloc(1, sizeof(digital_input_t));

   // Fill the common structure
   di->pin = pin;
   di->handler = reactor;
   
   if ( sense_mode )
   {
      // Direct
      di->direct.sense_mode = sense_mode;
      di->direct.filter = filter_value;

      // Set the sense detection - enabling the interrupt detection
      ioport_set_pin_sense_mode(
         ioport_pin_to_port_id(di->pin), 
         ioport_pin_to_index(di->pin)
      );
   }
   else
   {
      // Regular
      di->sampled.integrator_threshold = filter_value / DIGITAL_INPUT_SAMPLE_PERIOD;
   }

   // Append to the next
   digital_input_t **next = &_first;
   
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

   _react_ack_it = reactor_register(
      _digital_input_ack, DIGITAL_INPUT_ACK_PRIO, 1);

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
   // Read the value now
   _handle_pin_change_isr(0, PORTA.IN, PORTA.INTFLAGS);
}   

ISR(PORTB_PORT_vect)
{
   _handle_pin_change_isr(1, PORTB.IN, PORTA.INTFLAGS);
}

/**@}*/
/**@}*/
/**@} ---------------------------  End of file  --------------------------- */