#ifndef header_asx_slash_reactor_dot_hpp_has_already_been_included
#define header_asx_slash_reactor_dot_hpp_has_already_been_included
/**
 * A reactor scheduler, which also provide timer functions.
 * The reactor allow registering deferred callbacks (functions or methods),
 * and add them to the reactor.
 * Each handler have a priority attached. Prioritized handlers are executed first.
 * The timer function are executed in the context of a context of a timer reactor handler.
 *
 * Example:
 *
 * // Create a deferred function
 * void stuff(int a);
 *
 * // Create a deferred version which can be invoked from an interrupt with a input queue of 8
 * auto do_stuff = Deferred_q<Prio::medium, 8>(stuff);
 *
 * // Call do_stuff will schedule the execution from the reactor later on based on the prio level
 * do_stuff(1);
 * do_stuff(2);
 * do_stuff(3);
 *
 */
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdint.h>

#include <etl/type_traits.h>
#include <etl/queue.h>
#include <etl/delegate.h>
#include <etl/ratio.h>
#include <etl/vector.h>

#include "alert.h"
#include "debug.h"

#include <asx/chrono.hpp>
#include <asx/priority.hpp>

namespace asx
{
   /**
    * Base class for the reactor deferred objects.
    * A deferred object can be notified for a deferred execution in
    *  the reactor loop.
    * It hold a delegate pointer (to a method, lambda, function) to call
    *  from the reactor look when notified.
    */
   class IDeferred;

   template <size_t N> class BaseReactor
   {
      friend IDeferred;

   public:
      // Use std::conditional to determine the type of Mask
      using Mask = etl::conditional_t<
         (N > 16), uint32_t,
         etl::conditional_t<(N > 8), uint16_t, uint8_t>
      >;

      /// @brief For deferred calls, represent the future instance of the call
      using Future = uint32_t;
      BaseReactor() = default;

   protected:
      void add(Prio p, IDeferred *handler)
      {
         // Insert the new pair at the correct position
         handlers.emplace_back(Handler(p, handler));
      }

      void notify(Mask mask)
      {
         mask |= mask;
      }

   public:
      void run()
      {
         // Start with the msb
         Mask m = 1UL << (sizeof(Mask) * CHAR_BIT - 1);

         // Sort the vector based in the priority. Highest values are first
         etl::sort(
            handlers.begin(),
            handlers.end(),
            [](const Handler&a, const Handler&b) {
               return a.first > b.first;
            }
         );

         // Iterate the vector, and update all masks
         // Leading bits (msb) are used by the highest priorities
         // So counting the number of leading zeros gives the index in the vector
         for (const auto& [_, ptr] : handlers) {
            ptr->set_mask( m );
            m >>= 1;
         }

         // Of we go
         wdt_enable(WDTO_1S);

         // Atomically read and clear the notification flags allowing more
         //  interrupt from setting the flags which will be processed next time round
         while (true)
         {
            cli(); // Stop interrupt (this is reached when IT have finished)

            if ( mask == 0 ) // Anything to do?
            {
               debug_set(REACTOR_IDLE);

               // The AVR guarantees that sleep is executed before any pending interrupts
               sei();
               sleep_cpu();
               debug_clear(REACTOR_IDLE);
            }
            else
            {
               // Count leading zeros (The value must not be zero for this call)
               uint8_t index = __builtin_clz(mask);

               // Keep the system alive for as long as the reactor is calling handlers
               // We assume that if no handlers are called, the system is dead.
               wdt_reset();

               // Reset the mask
               mask &= ~handlers[index].second->mask;

               sei();

               // Invoke the handler
               handlers[index].second->process();
            }
         }
      }

      //template <typename F> Future call_later(chrono::steady_point::time_point, F);

      //template <typename F> void repeat(chrono::duration, F);

   private:
      using Handler = etl::pair<Prio, IDeferred*>;

      /** Maximum number of handlers */
      etl::vector<Handler, N> handlers;
      Mask mask;
   };

   #include "conf_reactor.hpp"

   // Default value for the max handler
   #ifndef REACTOR_MAX_HANDLES
   #  define REACTOR_MAX_HANDLES 32
   #endif

   using Reactor = class BaseReactor<REACTOR_MAX_HANDLES>;

   // For singleton
   extern Reactor reactor;
   Reactor &get_reactor();

   //template<Tp...> void call_later( duration d, Tp... );
   //template<Tp...> void repeat(duration d, Tp... );

} // Namespace asx

#endif // ndef header_asx_slash_reactor_dot_hpp_has_already_been_included


