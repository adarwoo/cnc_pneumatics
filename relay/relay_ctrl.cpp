#include <array>

#include "board.h"
#include "relay_ctrl.hpp"
#include <asx/ioport.hpp>

auto constexpr LED_A = PinDef{B, 1};
auto constexpr LED_B = PinDef{B, 0};
auto constexpr LED_C = PinDef{A, 2};

auto constexpr RELAY_A = PinDef{B, 3};
auto constexpr RELAY_B = PinDef{A, 7};
auto constexpr RELAY_C = PinDef{A, 6};


namespace relay {
   using namespace asx::ioport;

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
