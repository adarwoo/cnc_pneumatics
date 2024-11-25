#pragma once
#include "asx/reactor.hpp"

namespace asx {
   namespace modbus {
      enum class process_outcome_t : uint8_t {
         ignore,             // Wait for the stream to stop (3.5T) as it is not for us
         expecting_more,     // Char was processed, another is expected
         expecting_no_more,  // No more expected
      };

      enum class callback_outcome_t : uint8_t {
         reply_ready,            // A reply is ready to send
         unsupported_operation,  // Operation is not supported
         invalid_value,          // Value is out-of-range or not valid
      };

      enum class error_t : uint8_t
         illegal_function_code = 0x01, // Nodbus standard for illegal function code
         illegal_data_address = 0x02,
         illegal_data_value = 0x03,
         invalid_value,      // Value is out-of-range or not valid
      };

      class Crc {
         /// @brief Number of bytes received. Modbus limits to 256 bytes.
         uint8_t count;
         ///< The CRC for the currently received frame
         uint16_t crc;
         /// @brief Buffer of the last 2 bytes so they are not processed
         uint8_t n_minus_1;
         uint8_t n_minus_2;

      public:
         Crc() : {reset();}

         void reset() {
            crc = 0xffff;
            count = 0;
         }

         void operator()(uint8_t byte) {
            n_minus_2 = n_minus_1;
            n_minus_1 = byte;

            if ( count > 2 ) {
               update(n_minus_2);
            }
            else {
               ++count;
            }
         }

         void update(uint8_t byte) {
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

         bool check() {
            return (crc & 0xff == n_minus_1) && (crc >> 8 == n_minus_2);
         }
      };

      class DatagramProcessor {
         ///< State of datagram processing
         state_t dg_state;
         ///< Number of characters in the buffer
         uint8_t cnt;
         ///< Error code
         error_t error;

      protected:
         /**
          * The receiving buffer which holds the maximum possible number of characters.
          * Set by the derived class
          */
         uint8_t *buffer;

         ///< Crc
         Crc crc{};

      public:
         void reset() {
            dg_state = 0;
            cnt = 0;
            error = error_t::ok;
         }

         void update_crc(uint8_t byte) {
            n_minus_2_crc = n_minus_1_crc;
            n_minus_1_crc = crc;
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
      };

      template<class Proc, class Uart, template <asx::cpu_tick_t::rep> typename _Timer>
      class Slave {
         Proc dg_proc{};

      public:
         void on_timeout_t15() {
         }

         void on_rx_char(char c) {
            TIMER::start(); // Restart the timer (both)
            dg_proc.process_char(c);
         }

         void on_timeout_t35() {
         }
      };
   } // namespace modbus
} // end of namespace asx