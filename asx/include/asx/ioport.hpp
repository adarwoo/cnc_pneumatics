#include <ioport.h>
#include <stdint.h>
#include <avr/io.h>

namespace asx {
   namespace ioport {
      using mode_t = uint8_t;
      using port_t = uint8_t;
      using pin_t = uint8_t;
      using mask_t = uint8_t;

      enum class dir_t : uint8_t {
         in,
         out
      };

      enum class value_t :uint8_t {
         low = 1,
         high
      };

      enum class sense_t : uint8_t {
         interrupt_disable = PORT_ISC_INTDISABLE_gc,
         bothedges = PORT_ISC_BOTHEDGES_gc,
         rising = PORT_ISC_RISING_gc,
         falling = PORT_ISC_FALLING_gc,
         input_disabled = PORT_ISC_INPUT_DISABLE_gc,
         level_low = PORT_ISC_LEVEL_gc,
      };

      enum class invert_t : uint8_t {
         normal = 0,
         inverted = PORT_INVEN_bm
      };

      enum class pullup_t : uint8_t {
         disabled = 0,
         enabled = PORT_PULLUPEN_bm
      };

      enum class slewrate_limit_t : uint8_t {
        disabled = 0,
        enabled = 1
      };

      struct Port {
         PORT_t port;

         Port(PORT_t& _port) : port{_port} {}

        void set_slewrate( const slewrate_limit_t sr ) {
            if ( sr == slewrate_limit_t::enabled )
               port.PORTCTRL |= 1;
            else
               port.PORTCTRL &= ~1;
         }

         constexpr bool operator==(const Port& p) {
            return &(p.port) == &port;
         }
      };

      // Actual ports
      static inline auto A = Port(PORTA);
      static inline auto B = Port(PORTB);
      static inline auto C = Port(PORTC);

      // Create a dataless port - but a port
      template<Port &PORT, pin_t PIN>
      class Pin {
         static constexpr auto get_vport = []() {
            if constexpr ( PORT == A ) {
               return VPORTA;
            } else if constexpr ( PORT == B.port ) {
               return VPORTB;
            } else if constexpr ( PORT == C.port ) {
               return VPORTC;
            }

            static_assert(false, "Invalid port value");
         };
    public:

         static constexpr auto port = PORT;
         static constexpr auto vport = get_vport();
         static constexpr auto pin = PIN;

         static constexpr auto bitmask() -> mask_t {
            return uint8_t{1} << pin;
         }

         template<dir_t DIR, value_t VALUE=value_t::low, typename... OPTIONS>
         constexpr Pin(OPTIONS... opts) {
            vport().DIR = DIR;

            if constexpr(DIR == dir_t::in and VALUE == value_t::high or DIR == dir_t::out) {
               vport().OUT = VALUE;
            }

            // Compute configuration value for PINCTRL[pin]
            constexpr uint8_t default_config = 0; // Default config
            uint8_t config = (default_config | ... | process_option(opts)); // Fold expression

            // Apply the computed configuration
            //*(&port.port.PIN0CTRL + pin) = config;
         }

         constexpr auto operator()() -> bool {
            return vport().IN &= (~bitmask());
         }

         constexpr operator Port() const {
            return PORT;
         }

         constexpr auto set(const bool value=true) -> void {
            if (value) {
               vport() |= bitmask();
            } else {
               clear();
            }
         }

         constexpr auto clear() -> void {
            vport() &= ~bitmask();
         }

         constexpr auto toggle() -> void {
            vport() ^= bitmask();
         }

         template<typename... OPTIONS>
         constexpr auto set(OPTIONS... opts) -> void {
            // Compute configuration value for PINCTRL[pin]
            constexpr uint8_t default_config = 0; // Default config
            uint8_t config = (default_config | ... | process_option(opts)); // Fold expression


            //uint8_t v = port().PINCTRL[pin] & ~PORT_ISC_gm;
            //v |= static_cast<uint8_t>(sense);
            //port().PINCTRL[pin] = v;
         }


         //template<typename... T>
         //static constexpr uint8_t mask_of() {
         //   return (default_config | ... | process_option(opts));
        // }

      private:
         // Helper function to process each option
         constexpr uint8_t process_option(sense_t sense) const {
            return static_cast<uint8_t>(sense);
         }

         constexpr uint8_t process_option(invert_t invert) const {
            return static_cast<uint8_t>(invert);
         }

         constexpr uint8_t process_option(pullup_t pullup) const {
            return static_cast<uint8_t>(pullup); // Assuming pullup maps to a valid bitmask
         }

         // Helper function to process each option
         template<sense_t>
         constexpr uint8_t mask_of() const {
            return PORT_ISC_gm;
         }

         constexpr uint8_t mask(invert_t invert) const {
            return static_cast<uint8_t>(invert);
         }

         // Fallback for unsupported options
         template<typename T>
         constexpr uint8_t process_option(T) const {
            static_assert(always_false<T>, "Unsupported option type passed to Pin constructor");
            return 0;
         }

         template<typename T>
         static constexpr bool always_false = false;
      };

   } // End of ioport namespace
}


using namespace asx::ioport;

using LED0 = Pin<A, 4>;

auto led0 = LED0<dir_t::out>{};