/**
 * @addtogroup service
 * @{
 * @addtogroup digital_output
 * @{
 * @file
 * Implementation of the digital output  API
 * @author software@arreckx.com
 * @internal
 */
#include "digital_output.h"
#include "mem.h"

#include <ctype.h> // For isdigit

/************************************************************************/
/* Defines                                                              */
/************************************************************************/

/** @def DIGITAL_OUTPUT_MAX_CONCURRENT_SEQUENCE
 * Specify the maximum number of concurrent sequence
 */
#ifndef DIGITAL_OUTPUT_MAX_CONCURRENT_SEQUENCE
#  define DIGITAL_OUTPUT_MAX_CONCURRENT_SEQUENCE  8
#endif

/** @def DIGITAL_OUTPUT_PRIO
 * Override the reactor priority of the digital output handler
 */
#ifndef DIGITAL_OUTPUT_PRIO
#  define DIGITAL_OUTPUT_PRIO  reactor_prio_very_high
#endif

/************************************************************************/
/* Private types                                                        */
/************************************************************************/

/** Internal state machine for parsing the notes */
typedef enum {
   state_initial,
   state_duration,
   state_done
} _parsing_state_t;

/**
 * Holds the persistent information for the digital output
 */
typedef struct {
   ioport_pin_t pin;           ///< The pin to driver. This is the only information required
   uint16_t reference_period_ms;  ///< The reference period in ms, that is the whole period
   uint8_t current_duration_shift;///< The duration power of 2 division
   timer_instance_t timer;        ///< Keep the timer instance to cancel the timer
   const char *sequence;          ///< Pointer to the sequence
   const char *next;              ///< Pointer to the next operation
   bool repeat;
} _digital_output_t;

/************************************************************************/
/* Private variables                                                    */
/************************************************************************/

/** Common reactor handler */
reactor_handle_t _reactor;

/** Number of ports to handle */
uint8_t _number_of_outputs = 0;

/************************************************************************/
/* Private functions                                                    */
/************************************************************************/

/** Reset the timer counter such than the previous operation is without effect */
static inline void _cancel_sequence(timer_instance_t timer)
{
   if ( timer )
   {
      timer_cancel(timer);
      timer = 0;
   }
}

/** Parse the sequence string */
static inline bool _parse_next(_digital_output_t *out)
{
   const char *pNext = out->next;
   _parsing_state_t state = state_initial;
   
   while ( state != state_done )
   {
      char c = *pNext;

      if (c=='\0')
      {
         out->next = out->sequence;
         return false;
      }

      switch ( state )
      {
      case state_initial:
         if ( c == ' ' )
         {
            break;
         }
         
         if (c == '+')
         {
            ioport_set_pin_level(out->pin, true);
         }
         else if ( c == '-' )
         {
            ioport_set_pin_level(out->pin, false);
         }
         else
         {
            --pNext;
         }
         
         state = state_duration;
         break;
      case state_duration:
         if (isdigit(c))
         {
            out->current_duration_shift = c - '0';
         }
         else
         {
            --pNext;
         }
         
         state = state_done;
         break;
      default:
         break;
      }
      
      // Advance to the next char in every case (including to skip the space or slur)
      ++pNext;
   }
   
   out->next = pNext;
   return true;
}


/**
 * Handle the next sequence
 * The argument contains a pointer to the output structure.
 * At the end, may rearm for the next timer
 */
void _digital_output_reactor_handler(void *arg)
{
   _digital_output_t *output = (_digital_output_t*)arg;
   
   if ( _parse_next(output) || output->repeat )
   {
      output->timer = timer_arm(
         _reactor,
         timer_get_count_from_now(output->reference_period_ms >> output->current_duration_shift),
         0,
         output);
   }
}

/************************************************************************/
/* Public API                                                           */
/************************************************************************/


/**
 * The sequence is a open table container timer values. If a value is 0 or INT_MAX, the sequence terminates.
 * A 0 indicate the end - so the sequence does not repeat.
 * A -1 indicate a repeating sequence. If the number of elements is odd, the sequence is repeated inverted.
 * @param pin The pin to set
 * @param sequence
 * @param handle Storage for the timer handle
 */
void digitial_output_set(digital_output_t handle, bool value)
{
   _digital_output_t *out = (_digital_output_t *)handle;
   
   _cancel_sequence(out->timer);
   ioport_set_pin_level(out->pin, value);
}

void digitial_output_toggle(digital_output_t handle)
{
   _digital_output_t *out = (_digital_output_t *)handle;

   _cancel_sequence(out->timer);
   ioport_toggle_pin_level(out->pin);
}

/**
 * Driver the output using a pre-programmed sequence.
 * This is perfect for LEDs.
 * If a sequence is already running, the new sequence takes over immediately
 *
 * @param handle The handle to driver
 * @param reference_time The reference time from which the fractions are determined. 
 *                       Make it the duration of the longest item in the sequence for maximum accuracy
 * @param repeat If true, the sequence self repeats
 */
void digitial_output_start(digital_output_t handle, timer_count_t reference_time, const char *sequence, bool repeat)
{
   _digital_output_t *out = (_digital_output_t *)handle;

   _cancel_sequence(out->timer);
   out->sequence = sequence;
   out->next = sequence;
   out->repeat = repeat;
   out->reference_period_ms = reference_time;
   _digital_output_reactor_handler((void *)out);
}

/**
 * Call this once, after all output pins have been declared
 */
void digital_output_init(void)
{
   _reactor = reactor_register(_digital_output_reactor_handler, DIGITAL_OUTPUT_PRIO, DIGITAL_OUTPUT_MAX_CONCURRENT_SEQUENCE);
}


/**
 * Declare a digital output and make it manageable.
 * This call will dynamically allocate the memory for thtimer_inite structure.
 */
digital_output_t digital_output(ioport_pin_t pin)
{
   // Allocate some storage for this output
   _digital_output_t *output = (_digital_output_t *)mem_calloc(1, sizeof(_digital_output_t));
   
   // Initialize the output
   output->pin = pin;
   output->repeat=false;

   // We need to count in order to create a reactor with a queue big enough for all output at once
   ++_number_of_outputs;
   
   return (digital_output_t)output;
}


/**@}*/
/**@}*/
/**@} ---------------------------  End of file  --------------------------- */