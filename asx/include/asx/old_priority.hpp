#ifndef header_asx_slash_priority_dot_hpp_has_already_been_included
#define header_asx_slash_priority_dot_hpp_has_already_been_included

#include <stdint.h>

namespace asx
{
   /**
    * Priority levels used by the reactor
    */
   enum class Prio : uint8_t {
      idle = 0,
      low_minus       = 20,  low       = 30,  low_plus       = 40,
      medium_minus    = 70,  medium    = 80,  medium_plus    = 90,
      high_minus      = 120, high      = 130, high_plus      = 140,
      very_high_minus = 170, very_high = 180, very_high_plus = 190,
      realtime_minus  = 220, realtime  = 230, realtime_plus  = 240,
   };
}

#endif // ndef header_asx_slash_priority_dot_hpp_has_already_been_included