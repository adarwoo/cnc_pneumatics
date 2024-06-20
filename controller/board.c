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
#include <stdbool.h>

#include "sysclk.h"
#include "ioport.h"
#include "reactor.h"
#include "timer.h"
#include "digital_input.h"
#include "digital_output.h"
#include "piezzo.h"

#include "conf_board.h"


void board_init(void)
{
   // Configure the clock
   sysclk_init();
   
   /* This function is meant to contain board-specific initialization code
   * for, e.g., the I/O pins. The initialization can rely on application-
   * specific board configuration, found in conf_board.h.
   */

   /*
    * OC pins drive a PNP. Invert the pin, so the application logic is normal
    */
   ioport_set_pin_mode(OC_DOOR_CLOSED, PORT_INVEN_bm);
   ioport_set_pin_level(OC_DOOR_CLOSED, false);
   ioport_set_pin_dir(OC_DOOR_CLOSED, IOPORT_DIR_OUTPUT);
   
   ioport_set_pin_mode(OC_CHUCK_RELEASED, PORT_INVEN_bm);
   ioport_set_pin_level(OC_CHUCK_RELEASED, false);
   ioport_set_pin_dir(OC_CHUCK_RELEASED, IOPORT_DIR_OUTPUT);

   /*
    * Inputs
    */
   ioport_set_pin_dir(IN_CHUCK_OPEN, IOPORT_DIR_INPUT);
   ioport_set_pin_dir(IN_SPINDLE_AIR_BLAST, IOPORT_DIR_INPUT);
   ioport_set_pin_dir(IN_TOOLSET_AIR_BLAST, IOPORT_DIR_INPUT);
   ioport_set_pin_dir(IN_SOUNDER, IOPORT_DIR_INPUT);
   ioport_set_pin_dir(IN_BEEP, IOPORT_DIR_INPUT);
   ioport_set_pin_dir(IN_DOOR_OPEN_CLOSE, IOPORT_DIR_INPUT);

   /*
    * Outputs
    */
   ioport_set_pin_level(LED_CHUCK, false);
   ioport_set_pin_dir(LED_CHUCK, IOPORT_DIR_OUTPUT);

   ioport_set_pin_level(LED_DOOR_CLOSING, false);
   ioport_set_pin_dir(LED_DOOR_CLOSING, IOPORT_DIR_OUTPUT);

   ioport_set_pin_level(LED_DOOR_OPENING, false);
   ioport_set_pin_dir(LED_DOOR_OPENING, IOPORT_DIR_OUTPUT);

   ioport_set_pin_level(LED_FAULT, false);
   ioport_set_pin_dir(LED_FAULT, IOPORT_DIR_OUTPUT);
   
   // Driven by the OC WO2 (TCA0) - Default (no need for the TCAROUTEA mux register)
   ioport_set_pin_level(PIEZZO_DRIVE_PIN, false);
   ioport_set_pin_dir(PIEZZO_DRIVE_PIN, IOPORT_DIR_OUTPUT);
   
   
   /*
    * Init all services
    */
   reactor_init();
   timer_init();
   digital_output_init();
   digital_input_init();
   piezzo_init();

   // Promote the i2c interrupt
   CPUINT.LVL1VEC = TWI0_TWIM_vect_num;
}
