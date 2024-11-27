/*
 * Relay modbus device
 */
#include <logger.h>
#include <sysclk.h>
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
   void on_get_status(uint8_t index, uint8_t op) {
      LOG_TRACE("RELAY", "%d - %d", index, op);

      Datagram::pack( uint8_t{1} ); // Number of bytes returned

      if ( index == 255 ) {
         uint8_t value = relays[2].status();
         value <<=1;
         value |= relays[1].status();
         value <<=1;
         value |= relays[0].status();

         Datagram::pack(value);
      } else if ( index < 3 ) {
         Datagram::pack( relays[index].status() );
      } else {
         Datagram::reply_error(modbus::error_t::illegal_data_value);
      }
   }

   void on_set_single(uint8_t index, uint16_t operation) {
      LOG_TRACE("RELAY", "%d - %d", index, operation);

      switch ( operation ) {
         case 0x0000: relays[index].clr(); break;
         case 0xFF00: relays[index].set(); break;
         case 0x5500: relays[index].tgl(); break;
         default:                        break;
      }
   }

   void on_write_all(uint16_t operation) {
      LOG_TRACE("RELAY", "%d", operation);

      switch ( operation ) {
         case 0x0000: clr(); break;
         case 0xFF00: set(); break;
         case 0x5500: tgl(); break;
         default:          break;
      }
   }

   void on_read_version() {
      Datagram::pack( uint16_t{0x0001} );
   }
} // End of namespace relay


int main()
{
   sysclk_init();
   reactor::init();
   relay::modbus_slave::init();

   // Clean the relay LED after 2 seconds
   reactor::bind(relay::clean_led).delay(std::chrono::seconds{2});

   reactor::run();
}
