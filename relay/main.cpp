/*
 * Relay modbus device main entry point.
 * The relays are initialised by the static constructor
 */
#include <sysclk.h>
#include <asx/reactor.hpp>
#include <asx/ioport.hpp>

// Defines the modbus_slave
#include "modbus.hpp"

// Defines the relay
#include "relay_ctrl.hpp"

using namespace asx;
using namespace relay;
using namespace std::chrono;


using namespace asx::ioport;

using LED0 = Pin<A, 4, dir::in, pullup::enabled, sense::rising, invert::inverted>;
auto led0 = LED0{};


int main()
{
   // Configure the system clock according to the conf/conf_clock.h
   sysclk_init();

   // Initialise the reactor and the timer
   reactor::init();

   // Initialise the modbus slave template API
   modbus_slave::init();

   // Clean the relay LED after 2 seconds
   reactor::bind(clean_relay_leds).delay(2s);

   // Run the reactor/scheduler
   reactor::run();
}
