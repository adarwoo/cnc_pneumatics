#include <avr/interrupt.h>
#include <asx/reactor.hpp>

namespace usart {
   auto on_usart_rx_complete = reactor::null;
   auto on_usart_tx_complete = reactor::null;
   auto on_usart_data_ready = reactor::null;

   ISR(USART0_RXC_vect)
   {
      reactor::notify_from_isr(on_usart_rx_complete);
   }

   ISR(USART1_RXC_vect)
   {
      reactor::notify_from_isr(on_usart_rx_complete);
   }

   ISR(USART0_TXC_vect)
   {
      reactor::notify_from_isr(on_usart_tx_complete);
   }

   ISR(USART1_TXC_vect)
   {
      reactor::notify_from_isr(on_usart_tx_complete);
   }

   ISR(USART0_DRE_vect)
   {
      reactor::notify_from_isr(on_usart_data_ready);
   }

   ISR(USART1_DRE_vect)
   {
      reactor::notify_from_isr(on_usart_data_ready);
   }
}
