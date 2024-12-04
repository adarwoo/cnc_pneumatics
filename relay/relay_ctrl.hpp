/*
 * relay.hpp
 *
 * API to control the relays
 */
#include <asx/ioport.hpp>

namespace relay {

   using namespace asx::ioport;

   /**
    * Relay control
    * Drives the coil and the led.
    */
   class RelayCtrl {
      Pin led;
      Pin relay;

   public:
      /** When constructed, the LED is ON to test it */
      RelayCtrl( Pin _led, Pin _relay ) : led(_led), relay(_relay) {
         led.init(value::high, dir::out);
         relay.init(value::low, dir::out);
      }

      inline void set(bool close=true) {
         led.set(close);
         relay.set(close);
      }

      inline void clr() {
         led.clear();
         relay.clear();
      }

      inline void tgl() {
         relay.toggle();
         led.set(relay());
      }

      inline bool status() {
         return relay();
      }
   };

   /** Accessor */
   auto get_relay(uint8_t index) -> RelayCtrl&;

   /** Reset the led to the actual relay state */
   void clean_relay_leds();
}
