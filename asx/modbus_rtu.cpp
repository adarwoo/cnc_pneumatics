#include <boost/sml/sml.hpp>

using namespace sml; // Postfix Notation

namespace modbus {
   struct one_and_a_half_character_timeout {};
   struct t35_timeout {};
   struct demand_of_emission {}
   struct char_received {}

   struct SlaveSM {
      auto operator()() {
         return make_transition_table(
            , "*idle"_s + event<char_received> / reset_timeouts = "reception"_s
            , "idle"_s + event<demand_of_emission> = "emission"_s
            , "reception"_s + event<char_received> / reset_timeouts = "reception"_s
            , "reception"_s + event<char_received> / reset_timeouts = "control_and_waiting"_s
            , "control_and_waiting"_s + event<t35_timeout> / reset_timeouts = "idle"_s
            , "emission"_s + event<char_sent> [ more_chars] = "emission"_s
            , "emission"_s + event<t35_timeout> [ more_chars] = "emission"_s
         );
      }
   };

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

   enum class error_t :
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

      /**
       * Update the CRC. The internal CRC is always 2 bytes behind.
       * Simply call this operator for every received bytes (including the CRC).
       * Call @check when done.
       */
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

   template<typename Proc>
   class DatagramProcessor : public Proc {
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
   };

   template<class Proc, class Uart, template <asx::cpu_tick_t::rep> typename _Timer>
   class Slave {
      auto dg_proc = DatagramProcessor<Proc>{};
      auto crc = Crc{};
      auto sm = boost::sml::sm<SlaveSM>{};

   public:
      void on_rx_char(char c) {
          // Restart both T1.5 and T3.5 timers
         TIMER::start();

         // Handle the char
         auto result = dg_proc.process_char(c);

      }

      void on_timeout_t15() {
      }

      void on_timeout_t35() {
      }
   };
} // namespace modbus

