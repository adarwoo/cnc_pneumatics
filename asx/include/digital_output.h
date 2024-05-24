#ifndef digital_output_HAS_ALREADY_BEEN_INCLUDED
#define digital_output_HAS_ALREADY_BEEN_INCLUDED
/**
 * @file
 * Reactor API declaration
 * @addtogroup service
 * @{
 * @addtogroup piezzo
 * @{
 *****************************************************************************
 * Digital output API
 * The output can play a sequence (optionally repeating) to flash LEDs etc.
 * or be driven directly. When driven directly, the sequence is immediately
 * stopped.
 * A timer instance is used for every running sequence
 *
 * The sequence string format is as follow:
 * Spaces:
 * ------
 *    Are ignored.
 * State:
 * ------
 *    +: State is ON
 *    -: State is OFF
 *    X: Toggle the state
 * Duration
 * --------
 *    A duration of the sequence can be optionally added as a number 0 to 8
 *    The duration applies to the item immediately before, or repeats the previous item
 *       0 is full duration
 *       1 is a half duration (2^1=2)
 *       2 is a quarter duration (2^2=4)
 *       etc.
 * Example:
 * --------
 *    Let's create a SOS sequence
 *    +2-3 +2-3 +2-32   +0-2 +0-2 +0-22   +2-3 +2-3 +2-320
 *    +2-3 : On for 1/4 then off for 1/8 (a .)
 *    +2-32 : Last of the ... is on for 1/4 then off for 1/8 + 1/4 for the gap
 *
 * @author software@arreckx.com
 */
#include <stdint.h>
#include <stdbool.h>

#include "ioport.h"
#include "timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Handle for a digital out */
typedef void * digital_output_t;

/** Declare a digital output */
digital_output_t digital_output(ioport_pin_t);

/** Call once to initialize the handler */
void digital_output_init(void);
      
/** Set the output value */
void digitial_output_set(digital_output_t, bool);

/** Toggle the output */
void digitial_output_toggle(digital_output_t);

/** Drive a sequence given as a string */
void digitial_output_start(digital_output_t, timer_count_t, const char *, bool);

#ifdef __cplusplus
}
#endif

/** @} */
/** @} */
#endif /* digital_output_HAS_ALREADY_BEEN_INCLUDED */