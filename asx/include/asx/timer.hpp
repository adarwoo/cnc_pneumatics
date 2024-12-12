/**
 * C++ version of the timer API
 * Creates mapping with chrono and objectify the timer_instance_t which
 *  becomes Instance
 */
#pragma once

#include <timer.h>
#include <asx/chrono.hpp>
#include <asx/reactor.hpp>

namespace asx {
   namespace timer {
      ///< Null handle for C++
      constexpr auto null = timer_instance_t{TIMER_INVALID_INSTANCE};

      // Define a steady clock based on your embedded 1ms timer
      struct steady_clock {
         using duration = std::chrono::duration<uint32_t, std::milli>;
         using rep = duration::rep; // uint32_t
         using period = duration::period; // std::milli
         using time_point = std::chrono::time_point<steady_clock>;
         static constexpr bool is_steady = true;

         // Returns the current time as a time_point
         static time_point now() noexcept {
            return time_point(duration(timer_get_count()));
         }

         // Cast operator to convert duration to timer_count_t
         static timer_count_t to_timer_count(duration d) {
            return static_cast<timer_count_t>(d.count());
         }

         // Cast operator to convert time_point to timer_count_t
         static timer_count_t to_timer_count(time_point tp) {
            return static_cast<timer_count_t>(tp.time_since_epoch().count());
         }
      };

      ///< Shortcut for the C++ handle
      class Instance {
         timer_instance_t instance;
      public:
         // Constructor to initialize handle
         explicit Instance() : instance(null) {}

         // Constructor to initialize handle
         Instance(timer_instance_t inst) : instance(inst) {}

         // Cast operator to timer_instance_t
         operator timer_instance_t() const {
            return (timer_instance_t)instance;
         }

         // Assignment operator from timer_instance_t
         Instance& operator=(timer_instance_t inst) {
            instance = inst;
            return *this;
         }

      // Operations
      public:
         bool cancel() {
            return timer_cancel(instance);
         }
      };

      using duration = steady_clock::duration;
      using time_point = steady_clock::time_point;

      inline void init() {
         timer_init();
      }

      inline bool cancel(uint32_t timer_id) {
         return timer_cancel(timer_id);
      }
   }
} // End of asx namespace