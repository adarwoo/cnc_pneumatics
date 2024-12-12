#include <etl/limits.h> // For UINT_MAX

#include "alert.h"
#include "debug.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "asx/reactor.hpp"
#include "asx/deferred.hpp"


namespace asx
{
   Reactor reactor;

   Reactor &get_reactor()
   {
      return reactor;
   }
}
