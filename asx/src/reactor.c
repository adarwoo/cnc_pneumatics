/**
 * @addtogroup service
 * @{
 * @addtogroup reactor
 * @{
 *****************************************************************************
 * Implementation of the reactor pattern.
 * This reactor allow dealing with asynchronous events handled by interrupts
 *  within the time frame of the main application.
 * When no asynchronous operation take place, the micro-controller is put to
 *  sleep saving power.
 * The reactor cycle time can be monitored defining debug pins REACTOR_IDLE
 *  and REACTOR_BUSY
 *****************************************************************************
 * @file
 * Implementation of the reactor API
 * @author software@arreckx.com
 * @internal
 */
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "alert.h"
#include "debug.h"
#include "reactor.h"

#include "debug.h"

#include "conf_board.h"

/**
 * @def REACTOR_MAX_HANDLERS
 * Maximum number of handlers for the reactor. This defines defaults
 *  to 8 and can be overridden in the board_config.h file if more are
 *  required.
 */
#ifndef REACTOR_MAX_HANDLERS
   #define REACTOR_MAX_HANDLERS 16
#endif


/**
 * @def reactor_mask_t
 * Event a bits in a mask. The type is large enough to support the maximum number of reactors
 */
#if REACTOR_MAX_HANDLERS <= 16
typedef uint16_t reactor_mask_t;
#else
typedef uint32_t reactor_mask_t;
#endif


/** Holds all reactor handlers with mapping to the reaction mask */
typedef struct
{
   reactor_handler_t handler;
   uint8_t priority;
   reactor_mask_t mask;
   queue_t queue;
} reactor_item_t;

/** Temporary structure to sort items by priority */
typedef struct
{
   uint8_t index;
   uint8_t priority;
} priority_item_t;


/** @cond internal */
/** Holds all on-going notification flags. This must not be used directly */
volatile reactor_mask_t reactor_notifications;
/** @endcond */

/** Map the reaction position to the handler lookup table */
static reactor_handle_t _handle_lookup[REACTOR_MAX_HANDLERS] = {0};

/** Current number of handlers */
static uint8_t _next_handle = 0;

/** Keep an array of handlers whose position match the bit position of the handle */
static reactor_item_t _handlers[REACTOR_MAX_HANDLERS] = {0};

/** Lock new registrations */
static bool reactor_lock = false;

static volatile uint8_t DEBUG_INDEX;

/** Initialize the reactor API */
void reactor_init(void)
{
   // Use a debug pin if available
   debug_init(REACTOR_IDLE);
   debug_init(REACTOR_BUSY);
   
   // Fill the look-up with linear values so that the reactor can be used prior to run
   for (int i=0; i<_next_handle; ++i)
   {
      _handle_lookup[i] = i;
   }

   // Allow simplest sleep mode to resume very fast
   sleep_enable();
}

/**
 * Register a new reactor handler.
 * The handler is called once an interrupt or another reactor handler calls
 *  the notify function,
 * The priority determines which handlers are called first in a round-robin
 *  scheduler.
 * Providing the system has enough processing time, a handler should
 *  eventually be called.
 * However, low priority handler will suffer from more potential delay and
 *  jitter.
 * A queue can be associated with a handler for cases where multiple
 *  notification can occur at the same time.
 * 
 * @param handler Function to call when an event is ready for processing
 * @param priority Priority of the handler during round-robin scheduling
                   High priority handlers are handled first
 *
 */

reactor_handle_t reactor_register( const reactor_handler_t handler, uint8_t priority, uint8_t queue_size )
{
   alert_and_stop_if(reactor_lock != false);
   alert_and_stop_if(_next_handle == REACTOR_MAX_HANDLERS);
   
   _handlers[_next_handle].handler = handler;
   _handlers[_next_handle].priority = priority;
   
   // Create a temporary mask so that notification are possible before run
   // This mask will be updated once the reactor run starts
   _handlers[_next_handle].mask = 1 << _next_handle;
   
   // Queue
   queue_init(&_handlers[_next_handle].queue, queue_size);

   return _next_handle++;
}


/**
 * Interrupts are disable to avoid the or'ing to endup badly
 */
void reactor_notify( reactor_handle_t handle, void *data )
{
   cli();
   
   reactor_notifications |= _handlers[handle].mask;
   
   // If the queue is full - drop old data
   queue_ringpush(&_handlers[handle].queue, data);
   
   sei();
}

/** Sorting compare function */
static int compare_prio(const void *e1, const void *e2)
{
   uint8_t p1 = ((priority_item_t *)e1)->priority;
   uint8_t p2 = ((priority_item_t *)e2)->priority;
 
   if ( p1 < p2 )
   {
      return 1;
   }
 
   if ( p1 > p2 )
   {
      return -1;
   }
 
   return  0;
}


/**
 * Separate function to sort so variables are properly scoped
 */
static inline void _reactor_sort_by_priority(void)
{
   priority_item_t priorities[_next_handle];
   uint32_t sorted_notifications = 0;
 
   for (int i=0; i<_next_handle; ++i)
   {
      priorities[i].index = i;
      priorities[i].priority = _handlers[i].priority;
   }
   
   qsort(priorities, _next_handle, sizeof(priority_item_t), compare_prio);
      
   // Do it atomically in case interrupts are already pumping the queues   
   cli();
   
   // Iterate over all the reactor and set the mask for each
   for (uint8_t i=0; i<_next_handle; ++i)
   {
      uint8_t sorted_index = priorities[i].index;
      uint32_t mask = (1 << i);
      
      _handle_lookup[i] = sorted_index;
      _handlers[sorted_index].mask = mask;
      
      // Any pending notifications are shuffled to account for new ordering
      if ( reactor_notifications & (1 << sorted_index) )
      {
         sorted_notifications |= mask;
      }
      else
      {
         sorted_notifications &= (~mask);
      }
   }
   
   // Do not allow new registration now all is sorted
   reactor_lock = true;
   reactor_notifications = sorted_notifications;
   
   sei();
}


/** Process the reactor loop */
void reactor_run(void)
{
   size_t i;
   reactor_mask_t flags;

   // Sort all items by priority
   _reactor_sort_by_priority();

   // Atomically read and clear the notification flags allowing more
   //  interrupt from setting the flags which will be processed next time round
   while (true)
   {
      debug_clear(REACTOR_BUSY);
      cli();

      if ( reactor_notifications == 0 )
      {
         debug_set(REACTOR_IDLE);

         // The AVR guarantees that sleep is executed before any pending interrupts
         sei();
         sleep_cpu();
         debug_clear(REACTOR_IDLE);
      }
      else
      {
         // At least 1 flag set
         flags = reactor_notifications;
         //debug_set(REACTOR_BUSY);
         sei();

         // Handle the flags
         for ( i=0; i<_next_handle; ++i )
         {
            if ( flags & 1 )
            {
               reactor_item_t *item;
               void *data;

               // Keep the system alive for as long as the reactor is calling handlers
               // We assume that if no handlers are called, the system is dead.
               wdt_reset();

               cli();
               /************************************************************************/
               /* Start of critical section                                            */
               /************************************************************************/

               item = &(_handlers[_handle_lookup[i]]);
               alert_and_stop_if( ! queue_leftpop(&item->queue, &data) );
               
               // If the queue is not empty - leave the flag set to go back in it
               // The round-robin will still apply, and the next item in queue is
               // not necessarily the next
               if ( queue_is_empty(&item->queue) )
               {
                  // Reset the flag
                  reactor_notifications &= (~item->mask);
               }
               
               /************************************************************************/
               /* End of critical section                                              */
               /************************************************************************/
               sei();

               // Call the handler
               item->handler(data);
               
               // Apply round-robin strategy
               break;
            }

            // Move onto next notification
            flags>>=1;
         }
      }
   };
}

 /**@}*/
 /**@}*/
 /**@} ---------------------------  End of file  --------------------------- */