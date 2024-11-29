/**
 * Implement all modbus callback functions
 */
#include <logger.h>
#include "modbus.hpp"
#include "relay_ctrl.hpp"

using namespace asx;

namespace relay {

   //
   // Implement all the callbacks
   //
   void on_read_coils(uint8_t addr, uint8_t qty) {
      LOG_TRACE("RELAY", "%d - %d", addr, qty);

      Datagram::pack( uint8_t{1} ); // Number of bytes returned

      uint8_t value = get_relay(2).status();
      value <<=1;
      value |= get_relay(2).status();
      value <<=1;
      value |= get_relay(2).status();

      // If address is 0, keep all, if 1 remove the first etc..
      value >>= addr;
      
      // Mask to keep the count
      value &= (1 << qty) - 1;

      if ( qty > (3 - addr) ) {
         Datagram::reply_error(modbus::error_t::illegal_data_value);
      } else {
         Datagram::pack(value);
      }
   }

   void on_set_single(uint8_t index, uint16_t operation) {
      LOG_TRACE("RELAY", "%d - %d", index, operation);

      switch ( operation ) {
         case 0x0000: get_relay(index).clr(); break;
         case 0xFF00: get_relay(index).set(); break;
         case 0x5500: get_relay(index).tgl(); break;
         default:                        break;
      }
   }

   void on_set_multiple(uint8_t values) {
      LOG_TRACE("RELAY", "%.2x", values);

      for ( uint8_t i=0; i<3; ++i ) {
         get_relay(i).set( values & 1 );
         values >>= 1;
      }
   }

   void on_read_version() {
      Datagram::pack( uint16_t{0x0001} );
   }
} // End of namespace relay
