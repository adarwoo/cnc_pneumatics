#ifndef piezzo_HAS_ALREADY_BEEN_INCLUDED
#define piezzo_HAS_ALREADY_BEEN_INCLUDED
/**
 * @file
 * Reactor API declaration
 * @addtogroup service
 * @{
 * @addtogroup piezzo
 * @{
 *****************************************************************************
 * Piezzo sounder API
 * The piezzo can produce simple sound and music at different tempi.
 * It is a good practice to initialise in the board.c.
 * You can override the default timer used by this API which is TCB1 by setting
 * PIEZZO_TCB in the board.h.
 *
 * The music string format is as follow:
 * Separator:
 * ----------
 *    All notes are separated by 1 space exactly or by 1 '~' to slur the notes.
 *       A B C : Play La Si Do distinct notes
 *       A~A~C : Play the
 * Notes:
 * ------
 *    Notes are uppercase:
 *       C, D, E, F, G, A, B for Do, Re, Mi ... Si
 *       R is a rest and no sound is played for the duration
 *    A single alterations can be added to any note:
 *       's' for making the note sharp
 *       'b' for making the note flat
 * Octave shift
 * ------------
 *    The note can be played in the upper or lower octave.
 *    The change persist over to the next note. The initial octave is octave 5.
 *       x' / x'' for shifting up 1 or 2 octave
 *       x, / x,, for shifting down 1 or 2 octave
 * Duration
 * --------
 *    The duration of the note is initialy a quarter note.
 *    The duration can be added as a number 0 to 5
 *       0 is a full note (2^0=1)
 *       1 is a half note (2^1=2)
 *       2 is a quaver (2^2=4)
 *       3 is a crotchet (2^3=8)
 *       4, 5 are possible for 16th and 32nd
 * Example:
 * --------
 *    Ab3' C, Cs,2~Cs3 R R
 *
 * @author software@arreckx.com
 */

#include <stdint.h>

#include "timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/** To call once prior to using any other API */
void piezzo_init(void);

/** Play a music sequence given as a music string */
void piezzo_play( uint8_t tempo, const char *music );

/** Play a single tone on top of whatever is playing right now */
bool piezzo_start_tone(const char *tone, timer_count_t duration);

/** Stop playing */
void piezzo_stop(void);

/** Stop playing the tone */
void piezzo_stop_tone(void);


#ifdef __cplusplus
}
#endif

/** @} */
/** @} */
#endif /* ndef piezzo_HAS_ALREADY_BEEN_INCLUDED */