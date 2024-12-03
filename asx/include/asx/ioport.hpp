#include <cstdint>
#include <type_traits>

#include <avr/io.h>

namespace asx
{
   namespace ioport
   {
      using mode_t = uint8_t;
      using pin_t = uint8_t;
      using mask_t = uint8_t;

      enum class dir_t : uint8_t
      {
         in = 0,
         out = 1
      };

      enum class value_t : uint8_t
      {
         low = 0,
         high = 1
      };

      struct option_t {
         uint8_t value;
      };

      struct pinctrl_t : option_t {};

      // CRTP Base for Scoped Options
      template <typename Derived>
      struct scoped_option_t : pinctrl_t {
         constexpr explicit scoped_option_t(uint8_t v) : pinctrl_t{v} {}

         // Allow implicit conversion to uint8_t for ease of use
         constexpr operator uint8_t() const { return value; }
      };

      // Specialized Scoped Options
      struct sense_t : scoped_option_t<sense_t> {
         constexpr explicit sense_t(uint8_t v) : scoped_option_t(v) {}
      };

      static constexpr sense_t interrupt_disable{PORT_ISC_INTDISABLE_gc};
      static constexpr sense_t bothedges{PORT_ISC_BOTHEDGES_gc};
      static constexpr sense_t rising{PORT_ISC_RISING_gc};
      static constexpr sense_t falling{PORT_ISC_FALLING_gc};
      static constexpr sense_t input_disabled{PORT_ISC_INPUT_DISABLE_gc};
      static constexpr sense_t level_low{PORT_ISC_LEVEL_gc};

      struct invert_t : scoped_option_t<invert_t> {
         constexpr explicit invert_t(uint8_t v) : scoped_option_t(v) {}
      };

      static constexpr invert_t normal{0};
      static constexpr invert_t inverted{PORT_INVEN_bm};

      struct pullup_t : scoped_option_t<pullup_t> {
         constexpr explicit pullup_t(uint8_t v) : scoped_option_t(v) {}
      };

      static constexpr pullup_t disabled{0};
      static constexpr pullup_t enabled{PORT_PULLUPEN_bm};

      enum class slewrate_limit_t : uint8_t
      {
         disabled = 0,
         enabled = 1
      };

      template <uintptr_t BaseAddress>
      struct Port
      {
         static constexpr std::uintptr_t addr = BaseAddress;

         static PORT_t &port()
         {
            return *(reinterpret_cast<PORT_t *>(BaseAddress));
         }

         static constexpr VPORT_t &vport()
         {
            VPORT_t *retval = (VPORT_t *)(addr - 0x400);
            return *retval;
         }

         void set_slewrate(const slewrate_limit_t sr)
         {
            if (sr == slewrate_limit_t::enabled)
               port().PORTCTRL |= 1;
            else
               port().PORTCTRL &= ~1;
         }

         constexpr bool operator==(const Port &p)
         {
            return &(p.port) == &port;
         }
      };

      // Actual ports
      using A = Port<0x400>;
      using B = Port<0x420>;
      using C = Port<0x440>;

      // Create a dataless port - but a port
      template <typename PORT, pin_t PIN, dir_t _DIR, auto... OPTIONS>
      class Pin
      {
      public:
         using port = PORT;
         static constexpr auto pin = PIN;

         static constexpr auto bitmask() -> mask_t
         {
            return uint8_t{1} << pin;
         }

         constexpr Pin()
         {
            PORT::vport().DIR = (uint8_t)_DIR;

            //if constexpr ((_DIR == dir_t::in and VALUE == value_t::high) or (_DIR == dir_t::out))
            //{
            //   PORT::vport().OUT = (uint8_t)VALUE << PIN;
            //}

            // Compute the PINCTRL register value
            constexpr uint8_t pinctrl_value = compute_pinctrl();
            if constexpr (pinctrl_value != 0) {
                  register8_t *pinctrl = &(PORT::port().PIN0CTRL) + PIN;
                  *pinctrl = pinctrl_value;
            }
         }

         constexpr auto operator()() -> bool
         {
            return PORT::vport().IN &= (~bitmask());
         }

         constexpr auto set(const bool value = true) -> void
         {
            if (value)
            {
               PORT::vport() |= bitmask();
            }
            else
            {
               clear();
            }
         }

         constexpr auto clear() -> void
         {
            PORT::vport() &= ~bitmask();
         }

         constexpr auto toggle() -> void
         {
            PORT::vport() ^= bitmask();
         }

         // Configure the pin based on the options
         constexpr void configure_pin()
         {
            // Configure pin direction
            if constexpr (_DIR == dir_t::out)
            {
               PORT::port->DIR |= (1 << PIN);
            }
            else
            {
               PORT::port->DIR &= ~(1 << PIN);
            }
         }

      private:

    // Compute PINCTRL register value by summing options that inherit from pinctrl_t
    template<typename... OPTS>
    static constexpr uint8_t compute_pinctrl() {
        uint8_t result = 0;
        ((result |= static_cast<uint8_t>(std::is_base_of_v<pinctrl_t, OPTS> ? OPTS::value : 0)), ...);
        return result;
    }
      };

   } // End of ioport namespace
}

