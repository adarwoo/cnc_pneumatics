#ifndef header_asx_slash_deferred_dot_hpp_has_already_been_included
#define header_asx_slash_deferred_dot_hpp_has_already_been_included

#include <stdint.h>

#include <etl/type_traits.h>
#include <etl/utility.h>
#include <etl/queue.h>
#include <etl/delegate.h>
#include <etl/vector.h>

#include <boost/sml.hpp>

using namespace boost::sml;

#include <asx/priority.hpp>
#include <asx/reactor.hpp>

namespace asx
{
   /**
    * Base class for the reactor deferred objects.
    * A deferred object can be notified for a deferred execution in
    *  the reactor loop.
    * It hold a delegate pointer (to a method, lambda, function) to call
    *  from the reactor look when notified.
    */
   class IDeferred
   {
      friend Reactor;
   private:

      // Reactor mask for this Deferred instance
      Reactor::Mask mask;

   protected:
      void set_mask(Reactor::Mask const m) { mask = m; }

      void register_with_reactor(Prio p) {
         get_reactor().add(p, this);
      }

      void notify_reactor() {
         get_reactor().notify(mask);
      }

      /**
       * Called by the reator to call the callback
       */
      virtual void process() = 0;
   };

   template <size_t N, typename TArg, class A>
   class Deferred : public virtual IDeferred
   {
      etl::queue<TArg, N> args;
      A a;
      static_assert(N>0);

   public:
      template <class T, __BOOST_SML_REQUIRES(concepts::callable<void, T>::value)>
      Deferred(Prio p, const T &t) : A(aux::zero_wrapper<T>{t}) {
         register_with_reactor(p);
      }

      void operator()(TArg&& t) {
         args.emplace(t);
         notify_reactor();
      }

   public:
      virtual void process() final {
         TArg arg;
         args.pop_into(arg);
         delegate(arg);

         if ( not args.empty() ) {
            notify_reactor();
         }
      }
   };


   /**
    * Specialisation for deferred instances which
    *  do not take a parameter, and which will do not get queued.
    * Such objects can be notified multiple times before being handled once.
    * They're the fastest handlers and are well suited for interrupt handling.
    */
   template<>
   class Deferred<0, void, class A> : public virtual IDeferred
   {
      A a;

   public:
      template <class T, __BOOST_SML_REQUIRES(concepts::callable<void, T>::value)>
      Deferred(Prio p, const T &t) : A(aux::zero_wrapper<T>{t}) {
         register_with_reactor(p);
      }
   
      void operator()() {
         notify_reactor();
      }

   protected:
      virtual void process() final {
         // TODO
      }
   };

   /**
    * A queue less instance type for slow reactors or once reactor
    * Parameters passed in the last call wins over previous calls if
    * the callback could not be called in due time.
    * Avoid in situations where the same reactor can be invoked multiple time.
    */
   template <typename TArg>
   class Deferred<0, TArg> : public virtual IDeferred
   {
      TArg arg;
      etl::delegate<void(TArg)> delegate;

   public:
      Deferred( Prio p, etl::delegate<void(TArg)> _delegate ) : delegate( etl::forward(_delegate))
      {
         register_with_reactor(p);
      }

      void operator()(TArg t) {
         arg = t;
         notify_reactor();
      }

   protected:
      virtual void process() final {
         delegate(arg);
      }
   };

   /** Shortcut to the void deferred */
   using Deferred_v = Deferred<0, void>;

   /** Shortcut to the non-queue deferred */
   template<typename T> using Deferred_s = Deferred<1, T>;

   template<typename TArg>
   using delegate = etl::delegate<void(TArg)>;

   template<Prio p, size_t queue_size, typename TArg>
   auto deferred( delegate<TArg> &&_delegate )
   {
      if constexpr (queue_size == 0) {
         if constexpr (etl::is_void<TArg>::value) {
            return Deferred(p, _delegate);
         } else {
            return Deferred_s<TArg>(p, _delegate);
         }
      } else {
         return Deferred<queue_size, TArg>(p, _delegate);
      }
   }

} // Namespace asx

#endif // ndef header_asx_slash_deferred_dot_hpp_has_already_been_included