#ifndef TINY2_OSC_H_INCLUDED
#define TINY2_OSC_H_INCLUDED

#include <compiler.h>
#include <board.h>

/**
 * \weakgroup osc_group
 *
 * \section osc_group_errata Errata
 *   - Auto-calibration does not work on XMEGA A1 revision H and
 *     earlier.
 * @{
 */

//! \name Oscillator identifiers
//@{
//! 2 MHz Internal RC Oscillator
#define OSC_ID_RC20MHZ         CLKCTRL_OSC20MS_bm
//! 32 MHz Internal RC Oscillator
#define OSC_ID_ULP32KHZ        CLKCTRL_OSC32KS_bm
//! 32 KHz Internal RC Oscillator
#define OSC_ID_X32KHZ          CLKCTRL_XOSC32KS_bm
//! External Oscillator
#define OSC_ID_XOSC            CLKCTRL_EXTS_bm
//@}

//! \name External oscillator types
//@{
#define XOSC_TYPE_EXTERNAL        0      //!< External clock signal
#define XOSC_TYPE_32KHZ           2      //!< 32.768 kHz resonator on TOSC
#define XOSC_TYPE_XTAL            3      //!< 0.4 to 16 MHz resonator on XTAL
//@}

/**
 * \def CONFIG_XOSC_32KHZ_LPM
 * \brief Define for enabling Low Power Mode for 32 kHz external oscillator.
 */
#ifdef __DOXYGEN__
# define CONFIG_XOSC_32KHZ_LPM
#endif /* __DOXYGEN__ */

/**
 * \def CONFIG_XOSC_STARTUP
 * \brief Board-dependent value that determines the number of start-up cycles
 * for external resonators, based on BOARD_XOSC_STARTUP_US. This is written to
 * the two MSB of the XOSCSEL field of OSC.XOSCCTRL.
 *
 * \note This is automatically computed from BOARD_XOSC_HZ and
 * BOARD_XOSC_STARTUP_US if it is not manually set.
 */

//! \name XTAL resonator start-up cycles
//@{
#define XOSC_STARTUP_256      0   //!< 256 cycle start-up time
#define XOSC_STARTUP_1024     1   //!< 1 k cycle start-up time
#define XOSC_STARTUP_16384    2   //!< 16 k cycle start-up time
//@}

/**
 * \def CONFIG_XOSC_RANGE
 * \brief Board-dependent value that sets the frequency range of the external
 * oscillator. This is written to the FRQRANGE field of OSC.XOSCCTRL.
 *
 * \note This is automatically computed from BOARD_XOSC_HZ if it is not manually
 * set.
 */

//! \name XTAL resonator frequency range
//@{
//! 0.4 to 2 MHz frequency range
#define XOSC_RANGE_04TO2      OSC_FRQRANGE_04TO2_gc
//! 2 to 9 MHz frequency range
#define XOSC_RANGE_2TO9       OSC_FRQRANGE_2TO9_gc
//! 9 to 12 MHz frequency range
#define XOSC_RANGE_9TO12      OSC_FRQRANGE_9TO12_gc
//! 12 to 16 MHz frequency range
#define XOSC_RANGE_12TO16     OSC_FRQRANGE_12TO16_gc
//@}

/**
 * \def XOSC_STARTUP_TIMEOUT
 * \brief Number of us to wait for XOSC to start
 *
 * This is the number of slow clock cycles corresponding to
 * OSC0_STARTUP_VALUE with an additional 25% safety margin. If the
 * oscillator isn't running when this timeout has expired, it is assumed
 * to have failed to start.
 */

// If application intends to use XOSC.
#ifdef BOARD_XOSC_HZ
// Get start-up config for XOSC, if not manually set.
# ifndef CONFIG_XOSC_STARTUP
#  ifndef BOARD_XOSC_STARTUP_US
#   error BOARD_XOSC_STARTUP_US must be configured.
#  else
//! \internal Number of start-up cycles for the board's XOSC.
#   define BOARD_XOSC_STARTUP_CYCLES \
		(BOARD_XOSC_HZ / 1000000 * BOARD_XOSC_STARTUP_US)

#   if (BOARD_XOSC_TYPE == XOSC_TYPE_XTAL)
#    if (BOARD_XOSC_STARTUP_CYCLES > 16384)
#     error BOARD_XOSC_STARTUP_US is too high for current BOARD_XOSC_HZ.

#    elif (BOARD_XOSC_STARTUP_CYCLES > 1024)
#     define CONFIG_XOSC_STARTUP    XOSC_STARTUP_16384
#     define XOSC_STARTUP_TIMEOUT   (16384*(1000000/BOARD_XOSC_HZ))

#    elif (BOARD_XOSC_STARTUP_CYCLES > 256)
#     define CONFIG_XOSC_STARTUP    XOSC_STARTUP_1024
#     define XOSC_STARTUP_TIMEOUT   (1024*(1000000/BOARD_XOSC_HZ))

#    else
#     define CONFIG_XOSC_STARTUP    XOSC_STARTUP_256
#     define XOSC_STARTUP_TIMEOUT   (256*(1000000/BOARD_XOSC_HZ))
#    endif
#   else /* BOARD_XOSC_TYPE == XOSC_TYPE_XTAL */
#    define CONFIG_XOSC_STARTUP     0
#   endif
#  endif /* BOARD_XOSC_STARTUP_US */
# endif /* CONFIG_XOSC_STARTUP */

// Get frequency range setting for XOSC, if not manually set.
# ifndef CONFIG_XOSC_RANGE
#  if (BOARD_XOSC_TYPE == XOSC_TYPE_XTAL)
#   if (BOARD_XOSC_HZ < 400000)
#    error BOARD_XOSC_HZ is below minimum frequency of 400 kHz.

#   elif (BOARD_XOSC_HZ < 2000000)
#    define CONFIG_XOSC_RANGE    XOSC_RANGE_04TO2

#   elif (BOARD_XOSC_HZ < 9000000)
#    define CONFIG_XOSC_RANGE    XOSC_RANGE_2TO9

#   elif (BOARD_XOSC_HZ < 12000000)
#    define CONFIG_XOSC_RANGE    XOSC_RANGE_9TO12

#   elif (BOARD_XOSC_HZ <= 16000000)
#    define CONFIG_XOSC_RANGE    XOSC_RANGE_12TO16

#   else
#    error BOARD_XOSC_HZ is above maximum frequency of 16 MHz.
#   endif
#  else /* BOARD_XOSC_TYPE == XOSC_TYPE_XTAL */
#   define CONFIG_XOSC_RANGE     0
#  endif
# endif /* CONFIG_XOSC_RANGE */
#endif /* BOARD_XOSC_HZ */

#ifndef __ASSEMBLY__

static inline bool osc_is_ready(uint8_t id)
{
	return CLKCTRL.MCLKSTATUS & id;
}

/**
 * \brief Wait until the oscillator identified by \a id is ready
 *
 * This function will busy-wait for the oscillator identified by \a id
 * to become stable and ready to use as a clock source.
 *
 * \param id A number identifying the oscillator to wait for.
 */
static inline void osc_wait_ready(uint8_t id)
{
	while (!osc_is_ready(id)) {
		/* Do nothing */
	}
}


#endif

#endif /* TINY2_OSC_H_INCLUDED */
