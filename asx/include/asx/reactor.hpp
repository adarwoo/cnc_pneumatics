#ifndef reactor_hpp_HAS_ALREADY_BEEN_INCLUDED
#define reactor_hpp_HAS_ALREADY_BEEN_INCLUDED
/**
 * @file
 * C++ Reactor API declaration
 * @addtogroup service
 * @{
 * @addtogroup reactor
 * @{
 *****************************************************************************
 * TMP Reactor API.
 * The template version allow using functions taking arguments in the reactor
 *  and passing the argument from the reactor.
 * @author software@arreckx.com
 */
#include "reactor.h"

namespace reactor
{
   enum prio : uint8_t {
      low,
      high
   };

   ///< Shortcut for the C++ handle
   using handle = reactor_handle_t;
   using handler = reactor_handler_t;

   ///< Null handle for C++
   constexpr auto null = handle{255};

   template <typename Func>
   static constexpr auto bind(Func&& func, prio p = prio::low) -> handle {
      return reactor_register(reinterpret_cast<handler>(func), (reactor_priority_t)p);
   }

   static inline void init() { reactor_init(); }
   static inline void run() { reactor_run(); }
   static inline void notify_from_isr(handle on_xx) { reactor_null_notify_from_isr(on_xx); }
}

#endif // ndef reactor_hpp_HAS_ALREADY_BEEN_INCLUDED