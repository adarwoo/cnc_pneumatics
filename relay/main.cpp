/*
 * Relay modbus device main entry point.
 * The relays are initialised by the static constructor
 */
#include <sysclk.h>
#include <asx/reactor.hpp>
#include <asx/ioport.hpp>

// Defines the modbus_slave
#include "relay_ctrl.hpp"
#include "modbus.hpp"


using namespace asx;
using namespace relay;
using namespace std::chrono;

int main()
{
   // Configure the system clock according to the conf/conf_clock.h
   sysclk_init();

   // Initialise the reactor and the timer
   reactor::init();

   // Initialise the modbus slave template API
   modbus_slave::init();

   // Clean the relay LED after 2 seconds
   reactor::bind(relay::clean_leds).delay(2s);

   // Run the reactor/scheduler
   reactor::run();
}
