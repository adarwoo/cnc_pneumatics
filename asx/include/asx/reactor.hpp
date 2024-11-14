#pragma once
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

   // Concept to check if combined size of all arguments is within 4 bytes (32 bits)
   template <typename... Args>
   concept ReactorArgumentFits = (sizeof(Args) + ...) <= sizeof(void *);

   // Concept to validate function types based on argument size constraint
   template <typename Func>
   concept BindableFunction =
       std::is_invocable_v<Func> ||                       // No argument function
       (std::is_invocable_v<Func, Args...> && ReactorArgumentFits<Args...>);  // Arguments fit within 4 bytes

   template <BindableFunction Func>
   static constexpr auto bind(Func&& func, prio p = prio::low) -> handle {
      return reactor_register(reinterpret_cast<handler>(func), (reactor_priority_t)p);
   }

   // Helper function to pack two 16-bit values into a single 32-bit integer
   constexpr uint16_t pack(uint8_t a, uint8_t b) {
      return (static_cast<uint16_t>(a) << 8) | static_cast<uint16_t>(b);
   }

   // Notify function with no arguments
   void notify(handle h) {
      reactor_notify(h, nullptr);
   }

   // Notify function for one argument
   template <typename T>
   requires ReactorArgumentFits<T>
   void notify(handle h, T arg) {
      reactor_notify(h, reinterpret_cast<void*>(static_cast<uintptr_t>(arg)));
   }

   // Notify function for two arguments, packing them into a single 32-bit value
   template <typename T1, typename T2>
   requires ReactorArgumentFits<T1, T2>
   void notify(handle h, T1 arg1, T2 arg2) {
      uint32_t packed = pack(static_cast<uint8_t>(arg1), static_cast<uint8_t>(arg2));
      reactor_notify(h, reinterpret_cast<void*>(static_cast<uintptr_t>(packed)));
   }

   static inline void notify_from_isr(handle on_xx) { reactor_null_notify_from_isr(on_xx); }

   static inline void clear(handle h) { reactor_clear(h); }

   static inline void init() { reactor_init(); }
   static inline void run() { reactor_run(); }
}
