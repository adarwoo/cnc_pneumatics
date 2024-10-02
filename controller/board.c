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
#include "twim.h"

#include "conf_board.h"

void board_init(void)
{
   // Configure the clock
   sysclk_init();
   
   /* This function is meant to contain board-specific initialization code
   * for, e.g., the I/O pins. The initialization can rely on application-
   * specific board configuration, found in conf_board.h.
   */
   ioport_set_pin_level(RS485_XDIR_PIN, false);
   ioport_set_pin_dir(RS485_XDIR_PIN, true);

   // Masso input pins need the pull-up to avoid excess noise if not connected
   ioport_set_pin_mode(IN_MASSO0, PORT_PULLUPEN_bm);
   ioport_set_pin_mode(IN_MASSO1, PORT_PULLUPEN_bm);
   ioport_set_pin_mode(IN_MASSO2, PORT_PULLUPEN_bm);
   ioport_set_pin_mode(IN_MASSO3, PORT_PULLUPEN_bm);
   ioport_set_pin_mode(IN_MASSO4, PORT_PULLUPEN_bm);
   ioport_set_pin_mode(IN_MASSO5, PORT_PULLUPEN_bm);
   ioport_set_pin_mode(IN_MASSO6, PORT_PULLUPEN_bm);
   ioport_set_pin_mode(IN_MASSO7, PORT_PULLUPEN_bm);
   ioport_set_pin_mode(IN_MASSO8, PORT_PULLUPEN_bm);
   ioport_set_pin_mode(IN_MASSO9, PORT_PULLUPEN_bm);
   ioport_set_pin_mode(IN_MASSO10, PORT_PULLUPEN_bm);
   ioport_set_pin_mode(IN_MASSO11, PORT_PULLUPEN_bm);
   ioport_set_pin_mode(IN_MASSO12, PORT_PULLUPEN_bm);
   
   /*
    * Init all services
    */
   reactor_init();
   timer_init();
   
   // Initialize the ASF TWI
   twi_master_init(&TWI0);
   twi_master_enable(&TWI0);

   // Promote the i2c interrupt
   CPUINT.LVL1VEC = TWI0_TWIM_vect_num;
}
