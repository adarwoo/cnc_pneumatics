#include <asx/uart.hpp>
#include <asx/modbus_rtu.hpp>

#include "conf/conf_uart.hpp"
#include "datagram.hpp"
#include "relay.hpp"

namespace relay {
   // Create the modbus slave object
   using modbus_slave = asx::modbus::Slave<board::Uart, Datagram>;

   //
   // Implement all the callbacks
   //

   void on_get_single(uint8_t index)
      dg::pack( relays[index].status() );
   }

   void on_set_single(uint8_t relay_index, uint8_t operation) {
      switch ( operation ) {
      case 0x00:
         relays[index].clr(); break;
      case 0xFF:
         relays[index].set(); break;
      case 0x55:
         relays[index].tgl(); break;
      default:
         break;
      }
   }

   void on_write_all(uint8_t operation) {
      switch ( operation ) {
         case 0x00:
            clr(); break;
         case 0xFF:
            set(); break;
         case 0x55:
            tgl(); break;
         default:
            break;
      }
   }

   void on_read_version() {
      Datagram::pack( uint16_t{0x0001} );
   }
}

