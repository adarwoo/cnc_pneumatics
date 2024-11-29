/*
 * relay.hpp
 *
 * API to control the relays
 */
#include "ioport.h"

namespace relay {
   /**
    * Relay control
    * Drives the coil and the led.
    */
   class RelayCtrl {
      ioport_pin_t led;
      ioport_pin_t relay;

   public:
      /** When constructed, the LED is ON to test it */
      RelayCtrl( ioport_pin_t _led, ioport_pin_t _relay ) :
         led(_led), relay(_relay)
      {
         // Light LED on power-up
         ioport_set_pin_level(led, true);
         ioport_set_pin_level(relay, false);
         ioport_set_pin_dir(led, IOPORT_DIR_OUTPUT);
         ioport_set_pin_dir(relay, IOPORT_DIR_OUTPUT);
      }

      inline void set(bool close=true) {
         ioport_set_pin_level(led, close);
         ioport_set_pin_level(relay, close);
      }

      inline void clr() {
         ioport_set_pin_level(led, false);
         ioport_set_pin_level(relay, false);
      }

      inline void tgl() {
         bool onoff = status() ? false : true;
         ioport_set_pin_level(led, onoff);
         ioport_set_pin_level(relay, onoff);
      }

      inline bool status() {
         return ioport_get_pin_level(relay);
      }
   };

   /** Accessor */
   auto get_relay(uint8_t index) -> RelayCtrl&;

   /** Reset the led to the actual relay state */
   void clean_relay_leds();
}
