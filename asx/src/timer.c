/**
 * @file
 * [Timer](group__timer.html) service implementation
 * @internal
 * @addtogroup service
 * @{
 * @addtogroup timer
 * @{
 * @author gax
 */

#include <string.h>
#include <stdint.h>

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "timer.h"
#include "alert.h"
#include "reactor.h"
#include "board.h"


/************************************************************************/
/* Local macros                                                         */
/************************************************************************/

/************************************************************************/
/* Local types                                                          */
/************************************************************************/

/** Keep the arg and the callback together */
typedef struct _timer_future_t
{
	reactor_handle_t reactor;     ///< Reactor to invoke on expiry
	timer_instance_t instance;    ///< Timer instance
	timer_count_t count;          ///< Count when to trigger the timer
	timer_count_t repeat;         ///< Repeat delay
	void *arg;                    ///< Optional argument
} _timer_future_t;

/** One of the timer slot or -1 if not valid */
typedef volatile int_fast8_t _timer_slot_t;

/************************************************************************/
/* Local variables                                                      */
/************************************************************************/

/**
 * @def TIMER_MAX_CALLBACK
 * Default number of callbacks
 */
#ifndef TIMER_MAX_CALLBACK
#  define TIMER_MAX_CALLBACK 16
#endif

/** Invalid slot marker */
#define TIMER_INVALID_SLOT -1

/** Default the timer to TCB1 (the last one) */
#ifndef TIMER_TCB_NUMBER
#  define TIMER_TCB_NUMBER 1
#endif

#if TIMER_TCB_NUMBER == 0
#  define TIMER_TCB TCB0
#  define TIMER_TCB_INT_VECTOR TCB0_INT_vect
#else
#  define TIMER_TCB TCB1
#  define TIMER_TCB_INT_VECTOR TCB1_INT_vect
#endif

/** 
 * @def TIMER_PRIO
 * Assign a priority to the digital input reactor handler
 * Defaults to reactor_prio_very_high_plus
 */
#ifndef TIMER_PRIO
#  define TIMER_PRIO reactor_prio_very_high_plus
#endif


/**
 * Keep track of running timers
 * This array is sliding and sorted such as the first items are
 *  those that expire first.
 * This way, the interrupt handler
 */
static _timer_future_t _timer_future_sorted_list[TIMER_MAX_CALLBACK] = {0};

/** Free running counter. */
static volatile timer_count_t _timer_free_running_ms_counter = 0;

/** 0 based index to the next slot to use by the reactor loop */
static _timer_slot_t _timer_slot_reactor = TIMER_INVALID_SLOT;

/** 0 based index to the next slot to use upon IT */
static _timer_slot_t _timer_slot_active = 0;

/** 0 based index to the next available slot */
static _timer_slot_t _timer_slot_avail = 0;

/** Last instance given */
static timer_instance_t _timer_current_instance = TIMER_INVALID_INSTANCE;

/** The API Handle for the reactor */
static reactor_handle_t _timer_reactor_handle = 0;

/************************************************************************/
/* Private helpers                                                      */
/************************************************************************/

/** @return The index to the right */
static inline _timer_slot_t _timer_right_of(_timer_slot_t index)
{
	return index == (TIMER_MAX_CALLBACK - 1) ? 0 : index + 1;
}

/** @return The index to the left */
static inline _timer_slot_t _timer_left_of(_timer_slot_t index)
{
	return index == 0 ? (TIMER_MAX_CALLBACK - 1) : (index - 1);
}

/** @return The distance in tick from the current position */
static inline int32_t _timer_distance_of(timer_count_t from, timer_count_t to)
{
	int32_t retval = to - from;

	return retval;
}

/************************************************************************/
/* Local API                                                            */
/************************************************************************/

/** To be called from the reactor only. Look for expired jobs and process */
void timer_dispatch(void *);


/************************************************************************/
/* Public API                                                           */
/************************************************************************/

/**
 * Get the timer count.
 * @return The current free running counter
 */
timer_count_t timer_get_count(void)
{
	timer_count_t retval;

	// Stop a race between this accessor and the interrupt
	// Especially true for 32 bits counters which takes many assembly instructions
	sei();
	retval = _timer_free_running_ms_counter;
	cli();
	
	return retval;
}

/**
 * Ready the timer
 * Configure the timer and enable the interrupt.
 * Assumes irq are started
 */
void timer_init(void)
{
   size_t i;

	// Interrupt code for the AVR target only
	// This is initialised automatically for the simulator
#ifndef _WIN32
	// Use the Timer type B 1 to create a 1ms interrupt for the reactor
	TIMER_TCB.CNT = 0;												 // Reset the timer
	TIMER_TCB.CCMP = 10000;										 // 1ms timer
	TIMER_TCB.DBGCTRL = 0;											 // Stop the timer on a break point
	TIMER_TCB.CTRLA = TCB_CLKSEL_DIV2_gc | TCB_ENABLE_bm; // 10Mhz
	TIMER_TCB.CTRLB = TCB_CNTMODE_INT_gc;						 // Periodic interrupt mode
	TIMER_TCB.INTCTRL = TCB_CAPT_bm;							 // Turn on 'capture' interrupt
#endif

   // Reset the internal
   for ( i=0; i<TIMER_MAX_CALLBACK; ++i )
   {
      _timer_future_sorted_list[i].reactor = REACTOR_NULL_HANDLE;
   }

	// Register with the reactor
	_timer_reactor_handle = reactor_register(&timer_dispatch, TIMER_PRIO, 1);
}

/**
 * Compute the expiry timer count.
 * The value returned can be used with the timer_arm function.
 *
 * @param delayMs Delay to add to the count now
 * @return The count to hit in the future.
 */
timer_count_t timer_get_count_from_now(timer_count_t delayMs)
{
	// The returned value may roll over
	return (timer_get_count() + delayMs);
}

/**
 * Compute the time elapsed since a previous time
 *
 * @param count The previous timer count
 * @return The time elapsed since and from now
 */
timer_count_t timer_time_lapsed_since(timer_count_t count)
{
	return timer_get_count() - count;
}

/**
 * Arm a timer
 * This function checks for several conditions:
 *  * Now more slots!
 * The list is sorted to help the interrupt be short.
 * This function can be safely called from within an interrupt context.
 *
 * @param cb    Function to call on expiry. This function is called from within interrupt context
 * @param count Deadline value as a timer_count.
 *              This value is best computed by calling timer_get_count_from_now
 * @param repeat A timer count value to repeat the timer. It cannot be stopped.
 *              By adjusting the correct reactor priority, the repeat
 *              can form a round robin sequencer.
 *              If 0, does not repeat
 * @param arg   Extra argument passed to the caller.
 * 				 If NULL, the timer instance is passed as arg.
 * 				 If the timer is repeating, the initial value is passed every time
 * @return      The handle (slot position of the timer)
 */
timer_instance_t timer_arm(
	 reactor_handle_t reactor,
	 timer_count_t count,
	 timer_count_t repeat,
	 void *arg)
{
	_timer_slot_t insertPoint;
	_timer_slot_t i;

	timer_count_t now = timer_get_count();

	// Start from the active position
	insertPoint = _timer_slot_active;

	// Special case where the slots have met up with the active being used
	// Alert the user and drop the oldest slot
	if (
			(insertPoint == _timer_slot_avail) &&
			(_timer_future_sorted_list[insertPoint].reactor != REACTOR_NULL_HANDLE)
		)
	{
		alert();
	}

	// Look for the effective insertion position (sorted)
	while (insertPoint != _timer_slot_avail)
	{
		// The count is relative to the current position
		// Depending of where we are, we need to decide if the new count is a roll
		//  over
		if (
			_timer_distance_of(now, count) <
			_timer_distance_of(now, _timer_future_sorted_list[insertPoint].count))
		{
			break;
		}

		// Move onto next slot. Use modulus to loop round
		insertPoint = _timer_right_of(insertPoint);
	}

	// Shift all items to the right
	for (i = _timer_slot_avail; i != insertPoint;)
	{
		_timer_slot_t oneLeftOf = _timer_left_of(i);

		memcpy(
			 &_timer_future_sorted_list[i],			 // To
			 &_timer_future_sorted_list[oneLeftOf], // From
			 sizeof(_timer_future_t));

		i = oneLeftOf;
	}

	// Insert the new item
	_timer_future_t next = {
		 .reactor = reactor,
		 .count = count,
		 .instance = ++_timer_current_instance,
		 .arg = arg,
		 .repeat = repeat
	};

	_timer_future_sorted_list[insertPoint] = next;

	// Move next available slot
	_timer_slot_avail = _timer_right_of(_timer_slot_avail);

	return _timer_current_instance;
}


/**
 * Called by the timer ISR every 1ms
 * Only increment the timer if the dispatch has been called.
 * This guarantees, all handlers are called in time.
 */
ISR(TIMER_TCB_INT_VECTOR)
{
	// Clear the flag
	TIMER_TCB.INTFLAGS |= TCB_OVF_bm;

	++_timer_free_running_ms_counter;

	// Tell the reactor to process the tick
	reactor_notify(_timer_reactor_handle, NULL);
}

/**
 * Allow processing timer events in a reactor pattern.
 * This is called every ms and should be swift, but no race condition should
 *  occur since all is processed in the reactor
 */
void timer_dispatch(void *arg)
{
	// Grab an atomic copy of the time now
	timer_count_t timeNow = timer_get_count();

	// At least one pending timer
	while (_timer_slot_active != _timer_slot_avail)
	{
      _timer_future_t *pFuture = &_timer_future_sorted_list[_timer_slot_active];

		if (timeNow >= pFuture->count)
		{
			// Notify the reactor
			reactor_notify(pFuture->reactor, pFuture->arg);

			// Is it a repeating instance
			if (pFuture->repeat)
			{
				timer_arm(pFuture->reactor, pFuture->count + pFuture->repeat, pFuture->repeat, NULL);
			}

			// Move the pointer to the next item
			_timer_slot_active = _timer_right_of(_timer_slot_active);

         // Mark the reactor as NULL which could be required when the slots have met up
         pFuture->reactor = REACTOR_NULL_HANDLE;
		}
      else
      {
         break;
      }
	}
}

/**
 * Cancel a timer instance to reclaim the timer slot.
 * If the timer has not expire yet, cancels the timer.
 * Effectively stops repeating timers.
 *
 * The result is that the slot used by the timer becomes available at the end of the call, but
 * the timer processing reactior may still be invoked after this call.
 * Therefore, it is recommended for the reactor callback to check if the timer instance is
 * still valid prior to executing the code.
 * In system which may restart timers often (invalidating the previous timer), this
 * guarantees that only 1 timer slot is used and prevent resources exhaustion
 *
 * @param to_cancel Timer instance to cancel
 * @return true if the timer instance was canceled.
 *         If false, the timer may have already triggered the reactor.
 */
bool timer_cancel(timer_instance_t to_cancel)
{
	_timer_slot_t insertPoint;
	_timer_slot_t i;

		// Start from the active position
	insertPoint = _timer_slot_active;

	// At least one pending timer
	while (insertPoint != _timer_slot_avail)
	{
		_timer_future_t *pFuture = &_timer_future_sorted_list[insertPoint];

		if (pFuture->instance == to_cancel)
		{
			pFuture->reactor = REACTOR_NULL_HANDLE;

			// Now shift left all items to reclaim the space
			for ( i = insertPoint; i != _timer_slot_avail; )
			{
				_timer_slot_t oneRightOf = _timer_right_of(i);

				memcpy(
					&_timer_future_sorted_list[i],			// To
					&_timer_future_sorted_list[oneRightOf], // From
					sizeof(_timer_future_t));

				i = oneRightOf;
			}

			return true;
		}
      
      ++insertPoint;
	}
   
   return false;
}


/**@}*/
/**@} ---------------------------  End of file  --------------------------- */