/*
 * Relay modbus device
 */
#include <asx/reactor.hpp>
#include <asx/modbus_rtu.hpp>

#include "conf/conf_uart.hpp"
#include "datagram.hpp"
#include "relay_ctrl.hpp"

using namespace asx;

namespace relay {
   // Our relay modbus rtu slave templated class
   using modbus_slave = modbus::Slave<Datagram, board::Uart>;
   
   //
   // Implement all the callbacks
   //
   void on_get_single(uint8_t index) {
      Datagram::pack( relays[index].status() );
   }

   void on_set_single(uint8_t index, uint8_t operation) {
      switch ( operation ) {
         case 0x00: relays[index].clr(); break;
         case 0xFF: relays[index].set(); break;
         case 0x55: relays[index].tgl(); break;
         default:                        break;
      }
   }

   void on_write_all(uint8_t operation) {
      switch ( operation ) {
         case 0x00: clr(); break;
         case 0xFF: set(); break;
         case 0x55: tgl(); break;
         default:          break;
      }
   }

   void on_read_version() {
      Datagram::pack( uint16_t{0x0001} );
   }   
} // End of namespace relay


int main()
{
   reactor::init();
   relay::modbus_slave::init();
   
   reactor::run();
}
