#pragma once

#include <cstdint>
#include <type_traits>
#include <avr/io.h>

#include "sysclk.h"
#include <asx/reactor.hpp>
#include <asx/utils.hpp>

namespace asx {
   namespace usart {
      extern reactor::handle on_usart_rx_complete;
      extern reactor::handle on_usart_tx_complete;
      extern reactor::handle on_usart_data_ready;

      enum class width { _5, _6, _7, _8, _9 };
      enum class partity { odd, even, none };
      enum class stop { _1, _2 };

      // Options
      constexpr auto onewire = 1<<1;
      constexpr auto rs485   = 1<<2;;
      constexpr auto map_to_alt_position = 1<<3;
      constexpr auto disable_rx = 1<<4;
      constexpr auto disable_tx = 1<<5;

      template<int N, long BAUD, width W, partity P, stop S, int OPTIONS=0>
      class Uart {
         static_assert(N < 2, "Invalid USART number");

         static constexpr USART_t * const get_usart() {
            if constexpr (N == 0) {
               return &USART0;
            }

            return &USART1;
         }

         static constexpr uint16_t get_baud() {
            // Compute on full precision
            unsigned long baud = (64UL * F_CPU) / (BAUD / 16);
            return static_cast<uint16_t>(baud);
         }

         static consteval uint8_t get_ctrl_a() {
            uint8_t retval = 0;

            if (OPTIONS & rs485) {
               retval |= USART_RS485_bm;
            }

            if (OPTIONS & onewire) {
               retval |= USART_LBME_bm;
            }

            return retval;
         }

         static consteval uint8_t get_ctrl_b() {
            uint8_t retval = USART_RXEN_bm | USART_TXEN_bm | USART_RXMODE_NORMAL_gc;

            if (OPTIONS & onewire) {
               retval |= USART_ODME_bm;
            }

            if (OPTIONS & disable_rx) {
               retval &= (~USART_RXEN_bm);
            }

            if (OPTIONS & disable_rx) {
               retval &= (~USART_TXEN_bm);
            }

            return retval;
         }

         static consteval uint8_t get_ctrl_c() {
            uint8_t retval = USART_CMODE_ASYNCHRONOUS_gc;

            if (W == width::_5) {
               retval |= USART_CHSIZE_5BIT_gc;
            } else if (W == width::_6) {
               retval |= USART_CHSIZE_6BIT_gc;
            } else if (W == width::_7) {
               retval |= USART_CHSIZE_7BIT_gc;
            } else if (W == width::_8) {
               retval |= USART_CHSIZE_8BIT_gc;
            } else if (W == width::_9) {
               retval |= USART_CHSIZE_9BITH_gc;
            }

            if (P == partity::odd) {
               retval |= USART_PMODE_ODD_gc;
            } else if (P == partity::even) {
               retval |= USART_PMODE_EVEN_gc;
            }

            if (S == stop::_1) {
               retval |= USART_SBMODE_1BIT_gc;
            } else if (S == stop::_2) {
               retval |= USART_SBMODE_2BIT_gc;
            }

            return retval;
         }

      public:
         static void init() {
            if (OPTIKONS & map_to_alt_position) {
               if (N == 0) {
                  PORTMUX_USARTROUTEA |= PORTMUX_USART0_ALT1_gc;

                  if (is_onewire()) {
                     PORTA.PIN1CTRL |= PORT_PULLUPEN_bm;
                  } else {
                     VPORTA_DIR |= _BV(1);
                  }
               } else {
                  PORTMUX_USARTROUTEA |= 4; // Bug in AVR defs

                  if (is_onewire()) {
                     PORTC.PIN2CTRL |= PORT_PULLUPEN_bm;
                  } else {
                     VPORTC_DIR |= _BV(2);
                  }
               }
            } else {
               if (N == 0) {
                  if (is_onewire()) {
                     PORTB.PIN2CTRL |= PORT_PULLUPEN_bm;
                  } else {
                     VPORTB_DIR |= _BV(2);
                  }

               } else {
                  if (is_onewire()) {
                     PORTA.PIN1CTRL |= PORT_PULLUPEN_bm;
                  } else {
                     VPORTA_DIR |= _BV(1);
                  }
               }
            }

            get_usart()->CTRLA = get_ctrl_a();
            get_usart()->CTRLB = get_ctrl_b();
            get_usart()->CTRLC = get_ctrl_c();
            get_usart()->BAUD = get_baud();
         }

        static void write(uint8_t c) {
            get_usart()->TXDATAL = c;
        };


		 static void react_on_rx_complete_interrupt( reactor_handle_t reactor ) {
			on_usart_rx_complete = reactor;

			// Enable the interrupt
			get_usart()->CTRLA |= USART_RXCIE_bm;
		 }

		 static void react_on_tx_complete_interrupt( reactor_handle_t reactor ) {
			on_usart_tx_complete = reactor;

			// Enable the interrupt
			get_usart()->CTRLA |= USART_TXCIE_bm;
		 }

		 static void react_on_data_ready( reactor_handle_t reactor ) {
			on_usart_data_ready = reactor;

			// Enable the interrupt
			get_usart()->CTRLA |= USART_DREIE_bm;
		 }

      };
   } // end of namespace usart
} // end of namespace asx


// How to hook interrupts ?
