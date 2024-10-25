/*
 * pneumatic_hub.cpp
 *
 * Created: 06/05/2024 19:02:01
 * Author : micro
 */
#include "board.h"
#include "ioport.h"
#include "sysclk.h"

#include "timer.h"
#include "reactor.h"

#include <etl/array.h>

#include "asx/uart.hpp"
#include "asx/modbus.hpp"

#include "conf_uart.h"


/** Relay ios */
class RelayCtrl {
   ioport_pin_t led;
   ioport_pin_t relay;
public:
   RelayCtrl( ioport_pin_t _led, ioport_pin_t _relay ) : led(_led), relay(_relay) {
      // Light LED on power-up
      ioport_set_pin_level(led, true);
      ioport_set_pin_level(relay, false);
      ioport_set_pin_dir(led, IOPORT_DIR_OUTPUT);
      ioport_set_pin_dir(relay, IOPORT_DIR_OUTPUT);
   }

   void set(bool close) {
      ioport_set_pin_level(led, close);
      //ioport_set_pin_level(relay, close);
   }
};

auto relays = etl::array<RelayCtrl, 3> {
   RelayCtrl(LED_A, RELAY_A),
   RelayCtrl(LED_B, RELAY_B),
   RelayCtrl(LED_C, RELAY_C),
};


using Modbus = asx::Modbus<board::Uart>;

auto modbus = Modbus{};

int main()
{
   // Configure the clock
   sysclk_init();
   reactor_init();
   timer_init();
   modbus.init();

   reactor_run();
}
