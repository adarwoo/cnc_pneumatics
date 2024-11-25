/*
 * relay.hpp
 *
 * API to control the relays
 */
#include <array>

#include "board.h"
#include "ioport.h"

namespace relay
{
   /**
    * Relay control
    * Drives the coil and the led.
    */
   class RelayCtrl {
      ioport_pin_t led;
      ioport_pin_t relay;
      
   public:
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
         //ioport_set_pin_level(relay, close);
      }

      inline void clr() {
         ioport_set_pin_level(led, false);
         //ioport_set_pin_level(relay, close);
      }
      
      inline void tgl() {
         ioport_set_pin_level(led, !status());
         //ioport_set_pin_level(relay, close);
      }

      inline bool status() {
         return ioport_get_pin_level(led);
      }
   };

   static inline auto relays = std::array<RelayCtrl, 3> {
      RelayCtrl(LED_A, RELAY_A),
      RelayCtrl(LED_B, RELAY_B),
      RelayCtrl(LED_C, RELAY_C),
   };
   
   void set() {
      for (auto r : relays) {
         r.set();
      }
   }
   void clr() {
      for (auto r : relays) {
         r.clr();
      }
   }

   void tgl() {
      for (auto r : relays) {
         r.tgl();
      }         
   }

}
