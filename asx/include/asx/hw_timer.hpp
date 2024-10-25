#pragma once

#include <cstdint>
#include <type_trait>

#include <avr/io.h>

#include "sysclk.h"
#include "asx/reactor.hpp"

namespace timer
{
   extern reactor::handle on_timer_event;

   enum class mode : uint8_t {
      period, timeout, input_capture_on_event, input_capture_freq, input_capture_pwm, pwm, single_shot, pwm_8bits };

   // Define a concept that enforces the TimerType must be TCA_t or TCB_t
   template<typename T>
   concept IsValidTimerType = std::is_same_v<T, TCB_t> || std::is_same_v<T, TCA_t>;

   template <typename T>
   struct Counting {
      using Type = T;

      static constexpr Value maximum = std::numeric_limits<Type>::max();

      /** 8 for 8-bit timer, 16 for 16-bit timer */
      static constexpr uint8_t maximumPower2 = sizeof(Type) * 8;
   };

   using Counting8 = Counting<uint8_t>;
   using Counting16 = Counting<uint16_t>;

   // Concept to check if a type is a specialization of Counting
   template <typename T>
   concept CountingType = requires {
      typename T::Type;
      { T::maximum } -> std::convertible_to<typename T::Type>;
   };

   template <CountingType T>
   class TimerValue {
   public:
      using Value = typename T::value;

   private:
      Value value;

   public:
      constexpr static TimerValue maximum() { return Counting<Value>::maximum; }

      constexpr TimerValue(): value(0) {}
      constexpr TimerValue(Value v): value(v) {}
      constexpr TimerValue(const TimerValue<Prescaled> &v): value(v.value) {}
      constexpr TimerValue(const volatile TimerValue<Prescaled> &v): value(v.value) {}

      template <uint64_t v>
      constexpr TimerValue(::Time::Lit::Counts<v> counts): value(counts.getValue()) {}

      void operator= (Value v) volatile { value = v; }
      void operator= (const TimerValue &v) volatile { value = v.value; }
      void operator= (const volatile TimerValue &v) volatile { value = v.value; }

      constexpr bool operator> (const TimerValue &that) const volatile { return value > that.value; }
      constexpr bool operator> (const volatile TimerValue &that) const volatile { return value > that.value; }
      template <uint64_t v> constexpr TimerValue operator- (Lit::Counts<v>) const volatile { return value - v; }
      constexpr TimerValue operator- (const TimerValue &that) const volatile { return value - that.value; }
      constexpr TimerValue operator- (const volatile TimerValue &that) const volatile { return value - that.value; }

      constexpr Value getValue() const volatile { return value; }
      constexpr operator Value() const volatile { return value; }
   };


   // You must define a PrescalerMeta for your prescaler_t / prescaler combination,
   // with field "constexpr static uint8_t power2 = XXX", where XXX is the power of 2
   // by which the CPU clock is divided, e.g. 8 for a prescaler of 256.
   template<typename T, T V>
   struct PrescalerMeta {
      constexpr static uint8_t power2 = V;
   };

   template <typename T, typename P, P V>
   class Prescaled: public Counting<V> {
      using Meta = PrescalerMeta<P, V>;
   public:
      using Counting<T>::maximum;
      using Counting<T>::maximumPower2;

      using prescaler = P;

      static constexpr P prescaler = V;
      static constexpr uint8_t prescalerPower2 = Meta::power2;
   };

   /**
    * CRTP base for specialized timers in the system
    */
   template<typename IMPL>
   requires IsValidTimerType<typename IMPL::TimerType>
   struct ITimer
   {
      using type = typename IMPL::type;  // Access the timer type from the derived class

      constexpr void init() { static_cast<IMPL*>(this)->init(); }
      }
   };

   /**
    * Specialized instance of the timer A
    */
   class TimerA : public ITimer<TimerA>
   {
      using type = TCA_t;  // Specify the timer type as TCB_t

      static constexpr TCA_t * const get_timer() {
         return &TCA0;
      }
   };

   /**
    * Specialized instance of the timer B
    */
   template<int N>
   class TimerB : public ITimer<TimerB<N>>
   {
      using type = TCB_t;  // Specify the timer type as TCB_t

      static_assert(N < 2, "Invalid timer number");

      static constexpr TCB_t * const get_timer() {
         if constexpr (N == 0) {
            return &TCB0;
         }

         return &TCB1;
      }

      init() {
         get_timer->CTRLA = 0;
      }

      void set_compare_value(uint16_t value) {
         get_timer->
      }
   };
}


// Idea

// Create a class for functions

template<int CMP>
struct CompareTimer
{
   constexpr CompareTimer(
      TimerA &timer,
      const std::chrono::duration &duration
   ) {
      if constexpr (CMP === 0) {
         timer.get_timer()->CMP0 =
   }
};