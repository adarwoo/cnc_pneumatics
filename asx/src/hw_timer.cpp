#include <asx/hw_timer.hpp>
#include <avr/interrupt.h>
#include <asx/reactor.hpp>

namespace asx {
   namespace hw_timer {
      reactor::Handle on_timera_compare0;
      reactor::Handle on_timera_compare1;
      reactor::Handle on_timera_compare2;
      reactor::Handle on_timera_ovf;
      reactor::Handle on_timerb_compare;

      ISR(TCA0_CMP0_vect)
      {
         reactor::notify_from_isr(on_timera_compare0);
         TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP0_bm;
      }

      ISR(TCA0_CMP1_vect)
      {
         reactor::notify_from_isr(on_timera_compare1);
         TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm;
      }

      ISR(TCA0_CMP2_vect)
      {
         reactor::notify_from_isr(on_timera_compare2);
         TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP2_bm;
      }
   
      ISR(TCA0_OVF_vect)
      {
         reactor::notify_from_isr(on_timera_ovf);
         TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
      }

      ISR(TCB0_INT_vect)
      {
         reactor::notify_from_isr(on_timerb_compare);
         //TCA0.SINGLE.INTFLAGS |= TCB_I
      }
   } // Namespace hw_timer
} // Namespace asx
