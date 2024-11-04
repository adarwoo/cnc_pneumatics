#include <avr/interrupt.h>
#include <asx/reactor.hpp>

namespace asx {
   namespace uart {
      using dre_callback = void(*)();

      auto on_usart0_rx_complete = reactor::null;
      auto on_usart0_tx_complete = reactor::null;

      auto on_usart1_rx_complete = reactor::null;
      auto on_usart1_tx_complete = reactor::null;

      // These callbacks are managed by the Uart directly
      dre_callback dre_callback_uart0 = nullptr;
      dre_callback dre_callback_uart1 = nullptr;

      ISR(USART0_RXC_vect)
      {
         reactor::notify_from_isr(on_usart0_rx_complete);
      }

      ISR(USART1_RXC_vect)
      {
         reactor::notify_from_isr(on_usart0_rx_complete);
      }

      ISR(USART0_TXC_vect)
      {
         reactor::notify_from_isr(on_usart0_tx_complete);
         USART0.STATUS |= USART_RXCIE_bm;
      }

      ISR(USART1_TXC_vect)
      {
         reactor::notify_from_isr(on_usart1_tx_complete);
      }

      ISR(USART0_DRE_vect)
      {
         dre_callback_uart0();
      }

      ISR(USART1_DRE_vect)
      {
         dre_callback_uart1();
      }
   }
}