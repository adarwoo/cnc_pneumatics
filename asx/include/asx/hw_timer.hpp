#pragma once

#include <avr/io.h>

#include <cstdint>
#include <chrono>
#include <tuple>


#include "sysclk.h"
#include "asx/reactor.hpp"


namespace hw_timer
{
   extern reactor::handle on_timera_compare0;
   extern reactor::handle on_timera_compare1;
   extern reactor::handle on_timera_compare2;
   extern reactor::handle on_timerb_compare;

   using cpu_tick_t = std::chrono::duration<long long, std::ratio<1, F_CPU>>;

   ///< Convert any duration into CPU ticks
   constexpr auto to_ticks = [](auto duration) -> cpu_tick_t {
      return std::chrono::duration_cast<cpu_tick_t>(duration).count();
   };

   enum class mode : uint8_t {
      period,
      timeout,
      input_capture_on_event,
      input_capture_freq,
      input_capture_pwm,
      pwm,
      single_shot,
      pwm_8bits
   };

   template <typename T>
   struct Counting {
      using Type = T;

      static constexpr auto maximum = std::numeric_limits<Type>::max();

      /** 8 for 8-bit timer, 16 for 16-bit timer */
      static constexpr uint8_t maximumPower2 = sizeof(Type) * 8;
   };

   using Counting8 = Counting<uint8_t>;
   using Counting16 = Counting<uint16_t>;

   /**
    * Specialized instance of the timer A
    */
   template <long long COUNT, typename Duration =  std::chrono::milliseconds>
   struct TimerA
   {
      using type_t = TCA_t;  // Specify the timer type as TCA_t
      using value_t = Counting16;
      using self = TimerA;

      ///< Fixed period duration of this timer in CPU ticks
      static constexpr auto duration = cpu_tick_t{Duration{COUNT}};

      ///< @brief Possible prescaling values
      static constexpr TCA_SINGLE_CLKSEL_t clksel[] = {
         TCA_SINGLE_CLKSEL_DIV1_gc,
         TCA_SINGLE_CLKSEL_DIV2_gc,
         TCA_SINGLE_CLKSEL_DIV4_gc,
         TCA_SINGLE_CLKSEL_DIV8_gc,
         TCA_SINGLE_CLKSEL_DIV16_gc,
         TCA_SINGLE_CLKSEL_DIV64_gc,
         TCA_SINGLE_CLKSEL_DIV256_gc,
         TCA_SINGLE_CLKSEL_DIV1024_gc
      };

      ///< Actualling prescaling count - mapping clksel
      static constexpr long prescalers[] = {1, 2, 4, 8, 16, 64, 256, 1024};

      // Use statically only (there is only 1 timerA)
      TimerA() = delete;

      static constexpr TCA_SINGLE_t &TCA() {
         return *&(TCA0.SINGLE);
      }

      // Replace the lambda function with this constexpr function
      static constexpr auto set_prescaler_for_maximum_ticks() {
         return (
               duration.count() <= prescalers[0] * value_t::maximum) ? std::make_tuple(prescalers[0], clksel[0])
            : (duration.count() <= prescalers[1] * value_t::maximum) ? std::make_tuple(prescalers[1], clksel[1])
            : (duration.count() <= prescalers[2] * value_t::maximum) ? std::make_tuple(prescalers[2], clksel[2])
            : (duration.count() <= prescalers[3] * value_t::maximum) ? std::make_tuple(prescalers[3], clksel[3])
            : (duration.count() <= prescalers[4] * value_t::maximum) ? std::make_tuple(prescalers[4], clksel[4])
            : (duration.count() <= prescalers[5] * value_t::maximum) ? std::make_tuple(prescalers[5], clksel[5])
            : (duration.count() <= prescalers[6] * value_t::maximum) ? std::make_tuple(prescalers[6], clksel[6])
            : (duration.count() <= prescalers[7] * value_t::maximum) ? std::make_tuple(prescalers[7], clksel[7])
            : std::make_tuple(prescalers[0], clksel[0]); // Fallback (shouldn't happen with valid MaxTicks)
      }

      // Update the way you access prescaler and clk_setting
      static constexpr auto prescaler = std::get<0>(set_prescaler_for_maximum_ticks());
      static constexpr auto clk_setting = std::get<1>(set_prescaler_for_maximum_ticks());

      // Hold the 3 possible compare reactor handle
      template <typename... H>
      static constexpr void react_on_cmp(H... reactor_handles) {
         static_assert(sizeof...(H) <= 3, "Error: Too many handles, maximum is 3.");

         // Helper lambda to set each compare value (CMP0, CMP1, CMP2)
         auto set_rh = [](const int cmp_index, reactor::handle h) {
            if (cmp_index==0) {
               on_timera_compare0 = h;
               TCA().INTCTRL |= TCA_SINGLE_CMP0_bm;
               TCA().CTRLB |= TCA_SINGLE_CMP0EN_bm;
            } else if (cmp_index==1) {
               on_timera_compare1 = h;
               TCA().INTCTRL |= TCA_SINGLE_CMP1_bm;
               TCA().CTRLB |= TCA_SINGLE_CMP1EN_bm;
            } else {
               on_timera_compare2 = h;
               TCA().INTCTRL |= TCA_SINGLE_CMP2_bm;
               TCA().CTRLB |= TCA_SINGLE_CMP2EN_bm;
            }
         };

         auto indices = 0;
         (set_rh(indices++, reactor_handles), ...);
      }

      // Variadic template function to set multiple compare registers
      template <typename... Durations>
      static constexpr void set_compare(Durations... compare_values) {
         static_assert(sizeof...(compare_values) <= 3, "Error: Too many compare values, maximum is 3.");

         // Ensure each duration is less than or equal to MaxDuration
         //(static_assert(compare_values <= max_duration, "Error: Compare value exceeds max timer duration"), ...);

         // Helper lambda to set each compare value (CMP0, CMP1, CMP2)
         auto set_cmp = [&](int cmp_index, cpu_tick_t cmp_value) {
            (&(TCA().CMP0))[cmp_index] = cmp_value.count() / prescaler;
         };

         auto indices = 0;
         (set_cmp(indices++, compare_values), ...);
      }

      static void start() {
         TCA().CTRLA |= TCA_SINGLE_ENABLE_bm;
      }

      static void init() {
         TCA().CNT = 0;
         TCA().PER = duration.count() / prescaler;
         TCA().CTRLA = clk_setting;
         TCA().CTRLB = 0; // Normal mode
      }
   };

   /**
    * Specialized instance of the timer B
    */
   template<int N>
   class TimerB
   {
      using type = TCB_t;  // Specify the timer type as TCB_t
      using value_t = Counting16;

      static_assert(N < 2, "Invalid timer number");

      static TCB_t * const get_timer() {
         if constexpr (N == 0) {
            return &TCB0;
         }

         return &TCB1;
      }

      static void react_on_cmp( reactor::handle reactor ) {
         on_timerb_compare = reactor;
         // Enable the interrupt
         get_timer()->CTRLA |= TCB_ENABLE_bm;
      }

      // Variadic template function to set multiple compare registers
      template <typename Duration>
      constexpr void set_compare(const Duration& duration) {
         // Convert the given duration to cpu_tick_t based on F_CPU
         constexpr cpu_tick_t compare_value = std::chrono::duration_cast<cpu_tick_t>(duration);

         static_assert( compare_value.count() < (value_t::maximum * 2), "Number of ticks is too big" );

         // Set the timer prescaler
         auto* timer = get_timer();
         timer->CNT = 0;  // Reset the counter

         if (compare_value.count() < value_t::maximum) {
            timer->CTRLA = TCB_CLKSEL_DIV1_gc;
            timer->CCMP = compare_value.count();
         } else {
            timer->CTRLA = TCB_CLKSEL_DIV2_gc;
            timer->CCMP = compare_value.count() >> 1;
         }
      }

      // Use statically only (there is only 1 timerA)
      TimerB() = delete;
   };
}
