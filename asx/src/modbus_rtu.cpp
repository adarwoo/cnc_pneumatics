#include "asx/reactor.hpp"
#include "asx/modbus_rtu.hpp"

namespace asx {
   namespace modbus {
      Crc::Crc() {
         reset();
      }

      void Crc::reset() {
         crc = 0xffff;
         count = 0;
      }

      /**
       * Update the CRC. The internal CRC is always 2 bytes behind.
       * Simply call this operator for every received bytes (including the CRC).
       * Call @check when done.
       */
      void Crc::operator()(uint8_t byte) {
         n_minus_2 = n_minus_1;
         n_minus_1 = byte;

         if ( count > 2 ) {
            update(n_minus_2);
         }
         else {
            ++count;
         }
      }

      void Crc::update(uint8_t byte) {
         crc = crc ^ byte;

         for (unsigned char j = 1; j <= 8; ++j)
         {
            bool flag = crc & 0x0001;

            crc >>=1;

            if (flag)
            {
               crc ^= 0xa001;
            }
         }
      }

      bool Crc::check() {
         return ((crc & 0xff) == n_minus_1) && ((crc >> 8) == n_minus_2);
      }

      uint16_t Crc::update(etl::string_view view) {
         reset();
         
         for (auto c : view) {
            update(c);
         }

         return crc;
      }
   } // namespace modbus
} // namespace asx
