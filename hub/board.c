/*
 * board_init.c
 *
 * Created: 07/05/2024 15:36:23
 *  Author: micro
 */ 
/**
 * \file
 *
 * \brief User board initialization template
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip
 * Support</a>
 */

#include "ioport.h"
#include "conf_board.h"

void board_init(void)
{
	/* This function is meant to contain board-specific initialization code
	 * for, e.g., the I/O pins. The initialization can rely on application-
	 * specific board configuration, found in conf_board.h.
	 */

    // Force a zero on output the set as output
 	ioport_set_pin_level(IOPORT_TOOL_SETTER_AIR_BLAST, false);
    ioport_set_pin_dir(IOPORT_TOOL_SETTER_AIR_BLAST, IOPORT_DIR_OUTPUT);

    // Force a zero on output the set as output
    ioport_set_pin_level(IOPORT_CHUCK_CLAMP, false);
    ioport_set_pin_dir(IOPORT_CHUCK_CLAMP, IOPORT_DIR_OUTPUT);

    // Force a zero on output the set as output
    ioport_set_pin_level(IOPORT_SPINDLE_CLEAN, false);
    ioport_set_pin_dir(IOPORT_SPINDLE_CLEAN, IOPORT_DIR_OUTPUT);
    
    // Force a zero on output the set as output
    ioport_set_pin_level(IOPORT_DOOR_PUSH, false);
    ioport_set_pin_dir(IOPORT_DOOR_PUSH, IOPORT_DIR_OUTPUT);
    
    // Force a zero on output the set as output
    ioport_set_pin_level(IOPORT_DOOR_PULL, false);
    ioport_set_pin_dir(IOPORT_DOOR_PULL, IOPORT_DIR_OUTPUT);

    // Activate pull-ups on the input pin
    ioport_set_pin_dir(IOPORT_PRESSURE_READOUT, IOPORT_DIR_INPUT);

    // Activate trace pins for debug
    ioport_set_pin_dir(TRACE_INFO, IOPORT_DIR_OUTPUT);
    ioport_set_pin_dir(TRACE_WARN, IOPORT_DIR_OUTPUT);
    ioport_set_pin_dir(TRACE_ERR, IOPORT_DIR_OUTPUT);
}
