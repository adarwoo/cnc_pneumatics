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


/************************************************************************/
/* Define(s)                                                            */
/************************************************************************/

/** 
 * @def DIGITAL_INPUT_MAX_NUMBER_OF_INPUTS
 * Maximum number of inputs. Overwrite to add more as required.
 * Each additional incur a 6 bytes cost.
 * Defaults to 8
 */
#ifndef DIGITAL_INPUT_MAX_NUMBER_OF_INPUTS
#  define DIGITAL_INPUT_MAX_NUMBER_OF_INPUTS 8
#endif

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
/* Types                                                                */
/************************************************************************/

/** Hold digital input persistent information */
typedef struct
{
   /** An ioport pin to sample */
   ioport_pin_t pin;

   /** Reactor handler to call on change */
   reactor_handle_t handler;
   
   union 
   {
      struct 
      {
         /** Last known status of the filtered input */
         bool input;
         /** Internal value of the integrator */
         uint8_t integrator_threshold;
         /** Threshold value of the integrator */
         uint8_t integrator;
      } sampled;
      
      struct
      {
         /** Last known status of the filtered input */
         uint8_t sense_mode;
         /** Internal value of the integrator */
         timer_count_t filter;
      } direct;
   };
} _digital_input_t;


/************************************************************************/
/* Local variables                                                      */
/************************************************************************/

/** Structures to receive the input */
static _digital_input_t _digital_input[DIGITAL_INPUT_MAX_NUMBER_OF_INPUTS] = {0};
   
/** Points to the next regular (vs direct) digital input entry in the array */
static uint8_t _digital_input_next_sampled_entry = 0;

/** 
 * Points to the next direct digital input entry in the array from the bottom 
 * This pointer is signed to detect an overlap
 */
static int8_t _digital_input_next_direct_entry = DIGITAL_INPUT_MAX_NUMBER_OF_INPUTS-1;

/** Reactor for managing the sampling of the inputs */
static reactor_handle_t _react_handle_change;

/** Reactor for acknowledging the interrupts */
static reactor_handle_t _react_digital_input_ack;


/************************************************************************/
/* Private functions                                                    */
/************************************************************************/

/** 
 * Called by the timer at regular interval to sample the digital inputs
 */
void _digital_input_sample(void *arg)
{
   uint8_t i = 0;

   // Iterate over each input
   for ( ; i < _digital_input_next_sampled_entry; ++i )
   {
      _digital_input_t input = _digital_input[i];
      
      bool level = ioport_get_pin_level(input.pin);
      bool previous_input = input.sampled.input;
      
      if ( level )
      {
         if ( input.sampled.integrator < input.sampled.integrator_threshold )
         {
            ++input.sampled.integrator;
            
            if ( input.sampled.integrator < input.sampled.integrator_threshold )
            {
               input.sampled.input = true;
            }
         }
      }
      else
      {
         if ( input.sampled.integrator )
         {
            --input.sampled.integrator;

            if ( input.sampled.integrator == 0 )
            {
               input.sampled.input = false;
            }
         }
      }
      
      // Check the integrator result and invoke handler on change
      if ( input.sampled.input != previous_input )
      {
         reactor_notify(input.handler, &input);
      }
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
}


/** 
 * Common handler for both interrupts
 * Notify the handler, and arm a delayed interrupt acknowledgment if
 *  filtering is in place
 * @param index The port index. 0 for PORTA and 1 for PORTB
 */
static void _handle_pin_change_isr(uint8_t port_id, uint8_t port_value)
{
   uint8_t flags = port_id ? PORTB.INTFLAGS : PORTA.INTFLAGS;
   uint8_t pin_index = 0;
   
   // Check the int status to determine which one triggered.
   while ( flags & 1 )
   {
      // Given the pin position, and the port index, get the handler
      uint8_t i = DIGITAL_INPUT_MAX_NUMBER_OF_INPUTS - 1;
      
      // Grab the port value
      bool bit_value = port_value & 1;
   
      for ( ; i != _digital_input_next_direct_entry ; --i )
      {
         _digital_input_t input = _digital_input[i];

         // Get the correct pin         
         if ( ioport_pin_to_port_id(input.pin) == port_id && ioport_pin_to_index(input.pin) == pin_index )
         {
            pin_and_value_t pin_and_value = { .pin = input.pin, .value = bit_value };

            reactor_notify(input.handler, pin_and_value.as_arg);
         
            // Is filtering required?
            if ( input.direct.filter )
            {
               timer_arm(_react_digital_input_ack, input.direct.filter, 0, pin_and_value.as_arg);
            }
            else
            {
               // Acknowledge here and now
               _digital_input_ack(pin_and_value.as_arg);
            }
         }
      }
      
      // Move along!
      ++pin_index, flags >>= 1, port_value >>= 1;
   }
}   

/************************************************************************/
/* Public API                                                           */
/************************************************************************/

/** 
 * Create a digital input object 
 * @param p The port_io pin to watch
 * @param reactor A reactor handle which process any change
 * @param sense_mode If 0, the input is sampled, otherwise the value determine what
 *         input change cause the handler to respond.
 * @param filter_value Number of count to wait count when filtering, or number of ms to wait
 *         before acknowledging the interrupt. For direct sensing (interrupt mode), a value
 *        of 0 means the interrupt is acknowledged immediately.
 *        Note: This could lead to a DoS if the input is not properly managed
 */
void digital_input(ioport_pin_t pin, reactor_handle_t reactor, uint8_t sense_mode, uint8_t filter_value)
{
   // Make sure we have enough room left
   alert_and_stop_if( _digital_input_next_sampled_entry > _digital_input_next_direct_entry );

   // Get a pointer to the insert structure
   uint8_t index = sense_mode ? _digital_input_next_direct_entry-- : _digital_input_next_sampled_entry++;
   _digital_input_t *input = &_digital_input[index];
   
   // Fill the common structure
   input->pin = pin;
   input->handler = reactor;
   
   if ( sense_mode )
   {
      // Direct
      input->direct.sense_mode = sense_mode;
      input->direct.filter = (timer_count_t)filter_value;
   }
   else
   {
      // Regular
      input->sampled.integrator = filter_value;
   }
}

/** 
 * Initialize the digital input.
 * Make sure that all input are registered before calling this function
 * For C++, the inputs can be registered as global variables.
 * For C/C++, the board_init must register the inputs first, then call this function.
 * Register the 2 reactors for direct and regular inputs
 */
void digital_input_init(void)
{
   // There should be at least 1 entry to sample!
   if ( _digital_input_next_sampled_entry )
   {
      // Register the react
      _react_handle_change = reactor_register(
         _digital_input_sample, DIGITAL_INPUT_PRIO, _digital_input_next_sampled_entry);
         
      // Start a repeating timer to sample the inputs at regular interval
      timer_arm(_react_handle_change, timer_get_count_from_now(0), DIGITAL_INPUT_SAMPLE_PERIOD, NULL);
   }
   
   // Process the direct inputs
   uint8_t i = DIGITAL_INPUT_MAX_NUMBER_OF_INPUTS - 1;
   uint8_t max_ack = 0; // Count how many require a delay ack
   
   for ( ; i != _digital_input_next_direct_entry ; --i )
   {
      _digital_input_t input = _digital_input[i];
      
      // Set the sense detection - enabling the interrupt detection
      ioport_set_pin_sense_mode(
         ioport_pin_to_port_id(input.pin), 
         ioport_pin_to_index(input.pin)
      );
   }
   
   if ( max_ack )
   {
      _react_digital_input_ack = reactor_register(
         _digital_input_ack, DIGITAL_INPUT_ACK_PRIO, max_ack);
   }
}

/************************************************************************/
/* ISRs                                                                 */
/************************************************************************/
ISR(PORTA_PORT_vect)
{
   // Read the value now
   _handle_pin_change_isr(0, PORTA.IN);
}   

ISR(PORTB_PORT_vect)
{
   _handle_pin_change_isr(1, PORTB.IN);
}

/**@}*/
/**@}*/
/**@} ---------------------------  End of file  --------------------------- */