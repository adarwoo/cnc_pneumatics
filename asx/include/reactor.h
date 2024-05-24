#ifndef reactor_HAS_ALREADY_BEEN_INCLUDED
#define reactor_HAS_ALREADY_BEEN_INCLUDED
/**
 * @file
 * Reactor API declaration
 * @addtogroup service
 * @{
 * @addtogroup reactor
 * @{
 *****************************************************************************
 * Reactor API.
 * The reactor pattern allow to manage many asynchronous events
 *  within the same thread of processing.
 * This allow to process in the main function time and stack, events generated
 *  within interrupt code.
 * The events are prioritized such that the first handler that registers is
 *  always dealt with first.
 * This API is very simple, but deals with the complexity of atomically suspending
 *  the MPU in the main loop whilst processing interrupt outside of the interrupt
 *  context.
 * The interrupt do not need to hug the CPU time for long and simply notify the
 *  reactor that work is needed.
 * Therefore, all the work is done in the same context and stack avoiding
 *  nasty race conditions.
 * To use this API, simply register a handler with #reactor_register.
 * Then, if the main program needs to process the event, register a event handler
 *  using #reactor_register.
 * Finally, let the reactor loose once all the interrupts are up and running with
 *  #reactor_run.
 * @author software@arreckx.com
 */

#include <stdint.h>
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif


/** Invalid reactor handle */
#define REACTOR_NULL_HANDLE 255

/** Standard priorities for the reactor */
typedef enum {
   reactor_prio_idle = 0,
   reactor_prio_low_minus_minus = 10,
   reactor_prio_low_minus = 20,
   reactor_prio_low = 30,
   reactor_prio_low_plus = 40,
   reactor_prio_low_plus_plus = 50,
   reactor_prio_medium_minus_minus = 60,
   reactor_prio_medium_minus = 70,
   reactor_prio_medium = 80,
   reactor_prio_medium_plus = 90,
   reactor_prio_medium_plus_plus = 100,
   reactor_prio_high_minus_minus = 110,
   reactor_prio_high_minus = 120,
   reactor_prio_high = 130,
   reactor_prio_high_plus = 140,
   reactor_prio_high_plus_plus = 150,
   reactor_prio_very_high_minus_minus = 160,
   reactor_prio_very_high_minus = 170,
   reactor_prio_very_high = 180,
   reactor_prio_very_high_plus = 190,
   reactor_prio_very_high_plus_plus = 200,
   reactor_prio_realtime_minus_minus = 210,
   reactor_prio_realtime_minus = 220,
   reactor_prio_realtime = 230,
   reactor_prio_realtime_plus = 240,
   reactor_prio_realtime_plus_plus = 250,
} reactor_priorities_t;

/**
 * @typedef reactor_handle_t
 * A handle created by #reactor_register to use with #reactor_notify to
 *  tel the reactor to process the callback.
 */
typedef uint8_t reactor_handle_t;

/** Callback type called by the reactor when an event has been logged */
typedef void (*reactor_handler_t)(void *);

/** Initialize the reactor API */
void reactor_init(void);

/** Add a new reactor process */
reactor_handle_t reactor_register( const reactor_handler_t, reactor_priorities_t, uint8_t queue_size );

/**
 * Notify a handler should be invoke next time the loop is processed
 * Interrupt safe. No lock here since this is processed in normal
 * (not interrupt) context.
 */
void reactor_notify( reactor_handle_t handle, void * );


/** Process the reactor loop */
void reactor_run(void);

#ifdef __cplusplus
}
#endif

/** @} */
/** @} */
#endif /* ndef reactor_HAS_ALREADY_BEEN_INCLUDED */