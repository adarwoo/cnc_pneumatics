#pragma once

#include <asx/uart.hpp>

namespace board {

   /** Define the Usart to use by the rs485 */
   using Uart =
      asx::usart::Uart<
         1,                      // UART to use
         115200,                 // Baudrate
         asx::usart::width::_8,       // Width 5 to 9
         asx::usart::partity::odd,    // Parity Odd, Even, None
         asx::usart::stop::_1,        // Number of stop bits
         /* Add options as extra argument */
         asx::usart::rs485 | asx::usart::onewire
      >;
}
