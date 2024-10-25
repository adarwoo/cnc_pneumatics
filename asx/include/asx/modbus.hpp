#pragma once
#include "asx/reactor.hpp"

namespace asx
{
   template <typename UART>
   class Modbus
   {
	   using self = Modbus;
      static_assert(UART::is_rs485(), "Make sure to activate the rs485");

   protected:
	   static void on_rx_char( uint8_t ch );
	   static void on_timeout();

   public:
      static void init()
      {
         UART::init();
         UART::react_on_rx_complete_interrupt( *reactor::map( self::on_rx_char ) );
         // TIMER::init();
         // TIMER::enable_timer_match_interrupt( self::on_timeout ); // No reactor here
      }
   };

} // end of namespace asx