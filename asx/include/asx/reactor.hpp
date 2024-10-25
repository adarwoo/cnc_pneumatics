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
#include <tuple>
#include <type_traits>

#include <etl/queue.h>

#include "reactor.h"

namespace reactor
{
   enum prio : uint8_t {
      low,
      high
   };

   ///< Shortcut for the C++ handle
   using handle = reactor_handle_t;

   ///< Null handle for C++
   constexpr auto null = handle{255};

   // Type trait to extract argument types from a function pointer
   template <typename T>
   struct function_traits;

   // Specialization for function pointers
   template <typename R, typename... Args>
   struct function_traits<R(*)(Args...)> {
      using return_type = R;
      using argument_types = std::tuple<Args...>;
   };

   // handler which holds a tuple of arguments
   // One type per callback
   template <typename T>
   struct handler {
      using args_tuple_t = typename function_traits<T>::argument_types;  // Extract argument types from function
      T cb;  // Store the callback to the supplied method
      reactor_handle_t reactor_handle;
      args_tuple_t args;  // Store the arguments in a tuple

      constexpr handler(T callback, prio p = prio::low) : cb(callback) {
         reactor_handle = reactor_register(
            &reactor_callback,
            (reactor_priority_t)p
         );
      }

      constexpr reactor_handle_t operator*() {
         return reactor_handle;
      }

      // Store arguments when notify is called
      template <typename... Args>
      void operator()(Args... new_args) {
         args = std::make_tuple(new_args...);  // Store the passed arguments
         reactor_notify(reactor_handle, (void*)this);
      }

      // Call the stored callback with the stored arguments
      static void reactor_callback(void *arg) {
         auto self = reinterpret_cast<handler *>(arg);

         self->call_with_tuple(std::make_index_sequence<std::tuple_size<args_tuple_t>::value>{});
      }

   private:
      // Helper to expand the tuple and invoke the callback
      template <std::size_t... Is>
      void call_with_tuple(std::index_sequence<Is...>) {
         std::apply(this->cb, args);  // Call the callback with unpacked tuple arguments
      }
   };

   // Deduction guide to automatically deduce both the function type (T) and argument types (Args...)
   template <typename T>
   handler(T) -> handler<T>;

   // Now we define a `queue_handler` that uses a fixed-size queue
   template <typename T, size_t QueueSize>
   struct queue_handler {
      using args_tuple_t = typename function_traits<T>::argument_types;  // Extract argument types from function
      T cb;  // Store the callback
      reactor_handle_t reactor_handle;
      etl::queue<args_tuple_t, QueueSize> args_queue;  // Fixed-size queue of argument tuples

      constexpr queue_handler(T callback, prio p = prio::low) : cb(callback) {
         reactor_handle = reactor_register(
            &reactor_callback,
            (reactor_priority_t)p
         );
      }

      constexpr reactor_handle_t operator*() {
         return reactor_handle;
      }

      // Store arguments when notify is called (add to the queue)
      template <typename... Args>
      void operator()(Args... new_args) {
         if (!args_queue.full()) {
            args_queue.push(std::make_tuple(new_args...));  // Store the passed arguments in the queue
            reactor_notify(reactor_handle, (void*)this);
         } else {
            // Handle the case when the queue is full (optional logging or other action)
         }
      }

      // Call the stored callback with the stored arguments from the queue
      static void reactor_callback(void *arg) {
         auto self = reinterpret_cast<queue_handler *>(arg);

         if (!self->args_queue.empty()) {
            auto args = self->args_queue.front();
            self->args_queue.pop();  // Remove the front element after processing
            self->call_with_tuple(args, std::make_index_sequence<std::tuple_size<args_tuple_t>::value>{});

            // If the queue is not empty, notify the reactor
            if ( not self->args_queue.empty() )
            {
               reactor_notify(self->reactor_handle, (void*)self);
            }
         }
      }

   private:
      // Helper to expand the tuple and invoke the callback
      template <std::size_t... Is>
      void call_with_tuple(args_tuple_t& args, std::index_sequence<Is...>) {
         std::apply(this->cb, args);  // Call the callback with unpacked tuple arguments
      }
   };

   // Deduction guide for queue_handler
   template <typename T, size_t QueueSize>
   queue_handler(T) -> queue_handler<T, QueueSize>;

   // Factory function for either handler or queue_handler
   // For `handler` (no queue depth)
   template <typename T>
   auto map(T callback, prio p = prio::low) {
      return handler<T>(callback, p);
   }

   // Factory function for `queue_handler` (with queue depth)
   template <size_t QueueSize, typename T>
   auto map(T callback, prio p = prio::low) {
      return queue_handler<T, QueueSize>(callback, p);
   }

   inline void init() { reactor_init(); }
   inline void run() { reactor_run(); }
   inline void notify_from_isr(handle on_xx) { reactor_null_notify_from_isr(on_xx); }
}

#endif // ndef reactor_hpp_HAS_ALREADY_BEEN_INCLUDED