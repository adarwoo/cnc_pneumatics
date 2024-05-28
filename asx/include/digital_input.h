#ifndef digital_input_HAS_ALREADY_BEEN_INCLUDED
#define digital_input_HAS_ALREADY_BEEN_INCLUDED
/**
 * @file
 * Digital input API declaration
 * @addtogroup service
 * @{
 * @addtogroup digital_input
 * @{
 *****************************************************************************
 * Manage the digital input of the system.
 * The API offers 2 ways of sampling an input:
 *  1 - Direct
 *      The hardware senses the change, triggers an interrupt which ends-up
 *      being handled by the reactor.
 *      The API allow for a 'cool-off' period, so that no new interrupt can
 *      be triggered before some time
 *  2- Sampled
 *      The input is sampled at a regular interval, and integrated to filter
 *      the state. The reactor is called on each change
 * @author software@arreckx.com
 */
#include <stdint.h>
#include <stdbool.h>

#include "ioport.h"
#include "reactor.h"
#include "timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Composite structure holder the pin and its value
 * This is passed to all the digital input reactor handlers
 */
typedef union
{
   void *as_arg;
   
   struct 
   {
      ioport_pin_t pin;
      bool value;
   };
} pin_and_value_t;

/** Hold digital input persistent information */
typedef struct _digital_input_s
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

   /** Pointer to the next input */
   struct _digital_input_s *next;
} digital_input_t;

typedef digital_input_t *digital_input_handle_t;

/** Initialize the digital input API */
void digital_input_init(void);

/** Add an input for processing */
digital_input_handle_t digital_input(
   ioport_pin_t pin,
   reactor_handle_t reactor,
   uint8_t sense_mode,
   timer_count_t filter_value);
   
/** Grab the value directly */
bool digital_input_value( digital_input_handle_t );

#ifdef __cplusplus
}
#endif

#endif /* ndef digital_input_HAS_ALREADY_BEEN_INCLUDED */