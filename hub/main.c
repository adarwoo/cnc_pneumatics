/*
 * pneumatic_hub.cpp
 *
 * Created: 06/05/2024 19:02:01
 * Author : micro
 */ 

#include "board.h"
#include "pressure_mon.h"
#include "protocol.h"
#include "i2c_slave.h"

int main(void)
{
   // Initialize the board hardware (clocks, IOs, buses etc.)
   board_init();

   // Sample of 1 input (need digital input)
   pressure_mon_init();
   
   // The protocol is time sensitive - get it started
   protocol_init();
   
   // Turn on this board as an i2c slave
   i2c_slave_init(
      reactor_register(
         protocol_handle_traffic,
         reactor_prio_realtime,
         1 // Queue of 1 as realtime
      )
   );
   
   // Off we go!
   reactor_run();
}

