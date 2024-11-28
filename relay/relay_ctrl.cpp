#include <array>

#include "board.h"
#include "relay_ctrl.hpp"

namespace relay {
   auto relays = std::array<RelayCtrl, 3> {
      RelayCtrl(LED_A, RELAY_A),
      RelayCtrl(LED_B, RELAY_B),
      RelayCtrl(LED_C, RELAY_C),
   };

   /** Reset the led to the actual relay state */
   void clean_relay_leds() {
      for (auto r : relays) {
         r.set(r.status());
      }
   }

   auto get_relay(uint8_t index) -> RelayCtrl& {
       return relays[index];
   }
}
