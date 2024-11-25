#pragma once

#include <chrono>
#include "sysclk.h"

namespace asx
{
   namespace chrono {
      using cpu_tick_t = std::chrono::duration<long long, std::ratio<1, F_CPU>>;

      ///< Convert any duration into CPU ticks
      constexpr auto to_ticks = [](auto duration) -> cpu_tick_t {
         return std::chrono::duration_cast<cpu_tick_t>(duration).count();
      };
   }
}
