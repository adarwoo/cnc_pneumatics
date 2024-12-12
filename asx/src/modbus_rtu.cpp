#include <cstdio>
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
         if ( count >= 2 ) {
            update(n_minus_2);
         } else {
            ++count;
         }

         n_minus_2 = n_minus_1;
         n_minus_1 = byte;
      }

      void Crc::update(uint8_t byte) {
         crc ^= (uint16_t)byte;

         for (uint8_t j=8; j!=0; --j)
         {
            if (crc & 1) {
               crc >>= 1;
               crc ^= 0xA001;
            } else {
               crc >>= 1;
            }
         }
      }

      bool Crc::check() {
         uint8_t msb = crc >> 8;
         uint8_t lsb = crc & 0xff;

         bool retval = (msb == n_minus_1) && (lsb == n_minus_2);
         return retval;         
      }

      uint16_t Crc::update(std::string_view view) {
         reset();
         
         for (auto c : view) {
            update(c);
         }

         return crc;
      }
   } // namespace modbus
} // namespace asx
