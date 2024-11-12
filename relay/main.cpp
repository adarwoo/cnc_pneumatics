/*
 * pneumatic_hub.cpp
 *
 * Created: 06/05/2024 19:02:01
 * Author : micro
 */
#include <chrono>
#include <etl/array.h>
#include "board.h"
#include "ioport.h"
#include "sysclk.h"
#include "timer.h"

#include <asx/reactor.hpp>
#include <asx/hw_timer.hpp>
#include <asx/uart.hpp>

using timer = hw_timer::TimerA<1000, std::chrono::milliseconds>;

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

void on_timer_compare1() {
   static bool open = false;
   relays[0].set(open);
   open = not open;
}

void on_timer_compare2() {
   static bool open = false;
   relays[1].set(open);
   open = not open;
}

using namespace asx::uart;
using Rs485 = Uart<1, 9600, width::_8, parity::none, stop::_1, rs485 | onewire>;

void on_send_more() {
   static bool open = false;
   std::string_view data_to_send = "0123456789\n\r";

   Rs485::write(data_to_send);
   relays[2].set(open);
   open = not open;
}

int main()
{
   // Configure the clock
   sysclk_init();
   reactor_init();
#if 0
   timer::init();

   timer::set_compare(
      std::chrono::milliseconds{250},
      std::chrono::milliseconds{750}
   );

   timer::react_on_cmp(
      reactor::bind(on_timer_compare1),
      reactor::bind(on_timer_compare2)
   );

   timer::start();
#endif
   Rs485::init();
   Rs485::react_on_send_complete( reactor::bind(on_send_more) );
   on_send_more();

   reactor::run();
}
