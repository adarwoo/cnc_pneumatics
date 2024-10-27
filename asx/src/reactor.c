/**
 * @addtogroup service
 * @{
 * @addtogroup reactor
 * @{
 *****************************************************************************
 * Implementation of the reactor pattern.
 * This reactor allow dealing with asynchronous events handled by interrupts
 *  within the time frame of the main application.
 * When no asynchronous operation take place, the micro-controller is put to
 *  sleep saving power.
 * The reactor cycle time can be monitored defining debug pins REACTOR_IDLE
 *  and REACTOR_BUSY
 * This version does the sorting by position and first come first served
 *****************************************************************************
 * @file
 * Implementation of the reactor API
 * @author software@arreckx.com
 * @internal
 */
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>  // for CHAR_BIT

#include "utils/interrupt.h"

#include "alert.h"
#include "debug.h"
#include "reactor.h"

#include "debug.h"

/**
 * @def REACTOR_MAX_HANDLERS
 * Maximum number of handlers for the reactor. This defines defaults
 *  to 8 and can be overridden in the board_config.h file if more are
 *  required.
 */
#define REACTOR_MAX_HANDLERS 32

/** Holds all reactor handlers with mapping to the reaction mask */
typedef struct
{
   reactor_handler_t handler;
   void *arg;
} reactor_item_t;

/** Keep an array of handlers whose position match the bit position of the handle */
static reactor_item_t _handlers[REACTOR_MAX_HANDLERS] = {0};

/** Lock new registrations once the reactor is started */
static bool _reactor_lock = false;

/** Notification registry - use the GPIO for added performance */
#define _reactor_notifications (*(volatile uint32_t *)(&GPIO_GPIOR0))

/** Initialize the reactor API */
void reactor_init(void)
{
   // Use a debug pin if available
   debug_init(REACTOR_IDLE);
   debug_init(REACTOR_BUSY);

   // Allow simplest sleep mode to resume very fast
   sleep_enable();
}

/**
 * Register a new reactor handler.
 * The handler is called once an interrupt or another reactor handler calls
 *  the notify function,
 * The priority determines which handlers are called first in a round-robin
 *  scheduler.
 * Providing the system has enough processing time, a handler should
 *  eventually be called.
 * However, low priority handler will suffer from more potential delay and
 *  jitter.
 *
 * @param handler Function to call when an event is ready for processing
 * @param priority Priority of the handler during round-robin scheduling
                   High priority handlers are handled first
 *
 */
reactor_handle_t reactor_register(const reactor_handler_t handler, reactor_priority_t priority)
{
   alert_and_stop_if(_reactor_lock != false);

   // Initialize variables depending on priority
   uint8_t start = (priority == reactor_prio_low) ? REACTOR_MAX_HANDLERS - 1 : 0;
   uint8_t end = (priority == reactor_prio_low) ? 0 : REACTOR_MAX_HANDLERS;
   int8_t step = (priority == reactor_prio_low) ? -1 : 1;

   for (uint8_t i=start; i!=end; i += step)
   {
      if (_handlers[i].handler == NULL)
      {
         _handlers[i].handler = handler;
         return i;
      }
   }

   // Make sure a valid slot was found
   alert_and_stop();

   return 0;
}

/**
 * Helper which clears a bit of the GPIO register
 */
static inline void _clear_notification_bit(reactor_handle_t handle)
{
    // Inline assembly to clear the notification using CBI and a jump table
    asm volatile (
        "ldi r30, lo8(pm_clear_table_start)\n"  // Load the low byte of the jump table address into Z
        "ldi r31, hi8(pm_clear_table_start)\n"  // Load the high byte of the jump table address into Z
        "lsl %[handle]\n"                 // Multiply handle by 2 (each entry is 2 instructions)
        "add r30, %[handle]\n"            // Add handle to calculate the correct offset
        "adc r31, __zero_reg__\n"         // Handle carry in case of overflow
        "ijmp\n"                          // Indirect jump based on Z (r30:r31)
        "pm_clear_table_start:\n"
        "cbi %[gpior0], 0\n"          // Clear bit 0 in GPIOR0
        "rjmp endall\n"
        "cbi %[gpior0], 1\n"          // Clear bit 1 in GPIOR0
        "rjmp endall\n"
        "cbi %[gpior0], 2\n"          // Clear bit 2 in GPIOR0
        "rjmp endall\n"
        "cbi %[gpior0], 3\n"          // Clear bit 3 in GPIOR0
        "rjmp endall\n"
        "cbi %[gpior0], 4\n"          // Clear bit 4 in GPIOR0
        "rjmp endall\n"
        "cbi %[gpior0], 5\n"          // Clear bit 5 in GPIOR0
        "rjmp endall\n"
        "cbi %[gpior0], 6\n"          // Clear bit 6 in GPIOR0
        "rjmp endall\n"
        "cbi %[gpior0], 7\n"          // Clear bit 7 in GPIOR0
        "rjmp endall\n"
        "cbi %[gpior1], 0\n"          // Clear bit 0 in GPIOR1
        "rjmp endall\n"
        "cbi %[gpior1], 1\n"          // Clear bit 1 in GPIOR1
        "rjmp endall\n"
        "cbi %[gpior1], 2\n"          // Clear bit 2 in GPIOR1
        "rjmp endall\n"
        "cbi %[gpior1], 3\n"          // Clear bit 3 in GPIOR1
        "rjmp endall\n"
        "cbi %[gpior1], 4\n"          // Clear bit 4 in GPIOR1
        "rjmp endall\n"
        "cbi %[gpior1], 5\n"          // Clear bit 5 in GPIOR1
        "rjmp endall\n"
        "cbi %[gpior1], 6\n"          // Clear bit 6 in GPIOR1
        "rjmp endall\n"
        "cbi %[gpior1], 7\n"          // Clear bit 7 in GPIOR1
        "rjmp endall\n"
        "cbi %[gpior2], 0\n"          // Clear bit 0 in GPIOR2
        "rjmp endall\n"
        "cbi %[gpior2], 1\n"          // Clear bit 1 in GPIOR2
        "rjmp endall\n"
        "cbi %[gpior2], 2\n"          // Clear bit 2 in GPIOR2
        "rjmp endall\n"
        "cbi %[gpior2], 3\n"          // Clear bit 3 in GPIOR2
        "rjmp endall\n"
        "cbi %[gpior2], 4\n"          // Clear bit 4 in GPIOR2
        "rjmp endall\n"
        "cbi %[gpior2], 5\n"          // Clear bit 5 in GPIOR2
        "rjmp endall\n"
        "cbi %[gpior2], 6\n"          // Clear bit 6 in GPIOR2
        "rjmp endall\n"
        "cbi %[gpior2], 7\n"          // Clear bit 7 in GPIOR2
        "rjmp endall\n"
        "cbi %[gpior3], 0\n"          // Clear bit 0 in GPIOR3
        "rjmp endall\n"
        "cbi %[gpior3], 1\n"          // Clear bit 1 in GPIOR3
        "rjmp endall\n"
        "cbi %[gpior3], 2\n"          // Clear bit 2 in GPIOR3
        "rjmp endall\n"
        "cbi %[gpior3], 3\n"          // Clear bit 3 in GPIOR3
        "rjmp endall\n"
        "cbi %[gpior3], 4\n"          // Clear bit 4 in GPIOR3
        "rjmp endall\n"
        "cbi %[gpior3], 5\n"          // Clear bit 5 in GPIOR3
        "rjmp endall\n"
        "cbi %[gpior3], 6\n"          // Clear bit 6 in GPIOR3
        "rjmp endall\n"
        "cbi %[gpior3], 7\n"          // Clear bit 7 in GPIOR3
        "endall:\n"                       // End of the jump sequence
        :
        : [handle] "r" (handle), [gpior0] "I" (_SFR_IO_ADDR(GPIOR0)), [gpior1] "I" (_SFR_IO_ADDR(GPIOR1)), [gpior2] "I" (_SFR_IO_ADDR(GPIOR2)), [gpior3] "I" (_SFR_IO_ADDR(GPIOR3))
        : "r30", "r31"                    // Clobbered registers
    );
}

/**
 * Helper which sets a bit of the GPIO register
 */
static inline void _set_notification_bit( reactor_handle_t handle )
{
   // Inline assembly to handle the notification using SBI and jump table
   asm volatile (
        "ldi r30, lo8(pm_table_start)\n"  // Load the low byte of the jump table address into Z
        "ldi r31, hi8(pm_table_start)\n"  // Load the high byte of the jump table address into Z
        "lsl %[handle]\n"                 // Multiply handle by 2 (each entry is 2 instructions)
        "add r30, %[handle]\n"            // Add the handle to calculate the correct offset
        "adc r31, __zero_reg__\n"         // Handle carry in case of overflow
        "ijmp\n"                          // Indirect jump based on Z (r30:r31)
        "pm_table_start:\n"
        "sbi %[gpior0], 0\n"          // Set bit 0 in GPIOR0
        "rjmp endallclear\n"
        "sbi %[gpior0], 1\n"          // Set bit 1 in GPIOR0
        "rjmp endallclear\n"
        "sbi %[gpior0], 2\n"          // Set bit 2 in GPIOR0
        "rjmp endallclear\n"
        "sbi %[gpior0], 3\n"          // Set bit 3 in GPIOR0
        "rjmp endallclear\n"
        "sbi %[gpior0], 4\n"          // Set bit 4 in GPIOR0
        "rjmp endallclear\n"
        "sbi %[gpior0], 5\n"          // Set bit 5 in GPIOR0
        "rjmp endallclear\n"
        "sbi %[gpior0], 6\n"          // Set bit 6 in GPIOR0
        "rjmp endallclear\n"
        "sbi %[gpior0], 7\n"          // Set bit 7 in GPIOR0
        "rjmp endallclear\n"
        "sbi %[gpior1], 0\n"          // Set bit 0 in GPIOR1
        "rjmp endallclear\n"
        "sbi %[gpior1], 1\n"          // Set bit 1 in GPIOR1
        "rjmp endallclear\n"
        "sbi %[gpior1], 2\n"          // Set bit 2 in GPIOR1
        "rjmp endallclear\n"
        "sbi %[gpior1], 3\n"          // Set bit 3 in GPIOR1
        "rjmp endallclear\n"
        "sbi %[gpior1], 4\n"          // Set bit 4 in GPIOR1
        "rjmp endallclear\n"
        "sbi %[gpior1], 5\n"          // Set bit 5 in GPIOR1
        "rjmp endallclear\n"
        "sbi %[gpior1], 6\n"          // Set bit 6 in GPIOR1
        "rjmp endallclear\n"
        "sbi %[gpior1], 7\n"          // Set bit 7 in GPIOR1
        "rjmp endallclear\n"
        "sbi %[gpior2], 0\n"          // Set bit 0 in GPIOR2
        "rjmp endallclear\n"
        "sbi %[gpior2], 1\n"          // Set bit 1 in GPIOR2
        "rjmp endallclear\n"
        "sbi %[gpior2], 2\n"          // Set bit 2 in GPIOR2
        "rjmp endallclear\n"
        "sbi %[gpior2], 3\n"          // Set bit 3 in GPIOR2
        "rjmp endallclear\n"
        "sbi %[gpior2], 4\n"          // Set bit 4 in GPIOR2
        "rjmp endallclear\n"
        "sbi %[gpior2], 5\n"          // Set bit 5 in GPIOR2
        "rjmp endallclear\n"
        "sbi %[gpior2], 6\n"          // Set bit 6 in GPIOR2
        "rjmp endallclear\n"
        "sbi %[gpior2], 7\n"          // Set bit 7 in GPIOR2
        "rjmp endallclear\n"
        "sbi %[gpior3], 0\n"          // Set bit 0 in GPIOR3
        "rjmp endallclear\n"
        "sbi %[gpior3], 1\n"          // Set bit 1 in GPIOR3
        "rjmp endallclear\n"
        "sbi %[gpior3], 2\n"          // Set bit 2 in GPIOR3
        "rjmp endallclear\n"
        "sbi %[gpior3], 3\n"          // Set bit 3 in GPIOR3
        "rjmp endallclear\n"
        "sbi %[gpior3], 4\n"          // Set bit 4 in GPIOR3
        "rjmp endallclear\n"
        "sbi %[gpior3], 5\n"          // Set bit 5 in GPIOR3
        "rjmp endallclear\n"
        "sbi %[gpior3], 6\n"          // Set bit 6 in GPIOR3
        "rjmp endallclear\n"
        "sbi %[gpior3], 7\n"          // Set bit 7 in GPIOR3
        "endallclear:\n"                       // End of the jump sequence
        :
        : [handle] "r" (handle), [gpior0] "I" (_SFR_IO_ADDR(GPIOR0)), [gpior1] "I" (_SFR_IO_ADDR(GPIOR1)), [gpior2] "I" (_SFR_IO_ADDR(GPIOR2)), [gpior3] "I" (_SFR_IO_ADDR(GPIOR3))
        : "r30", "r31"                    // Clobbered registers
   );
}

/**
 * Interrupts are disabled for atomic operations
 * This function can be called from within interrupts
 */
void reactor_notify( reactor_handle_t handle, void *data )
{
   irqflags_t flags = cpu_irq_save();

   _handlers[handle].arg = data;
   _set_notification_bit(handle);

   cpu_irq_restore(flags);
}

void reactor_null_notify_from_isr( reactor_handle_t handle )
{
   _set_notification_bit(handle);
}

/** Process the reactor loop */
void reactor_run(void)
{
   // Set the watchdog which is reset by the reactor
   // If the timer is uses, the watchdog would be refreshed every 1ms, but otherwise, we don't know
   // There is no need for too aggressive timings
   wdt_enable(WDTO_1S);

   // Atomically read and clear the notification flags allowing more
   //  interrupt from setting the flags which will be processed next time round
   while (true)
   {
      debug_clear(REACTOR_BUSY);
      cli();

      if ( _reactor_notifications == 0 )
      {
         debug_set(REACTOR_IDLE);

         // The AVR guarantees that sleep is executed before any pending interrupts
         sei();
         sleep_cpu();
         debug_clear(REACTOR_IDLE);
      }
      else
      {
         // At least 1 flag set
         uint8_t pos = __builtin_ctzl(_reactor_notifications);

         // Clear the flag before calling - so it could be set again by the caller
         _clear_notification_bit(pos);

         sei();

         _handlers[pos].handler(_handlers[pos].arg);

         // Keep the system alive for as long as the reactor is calling handlers
         // We assume that if no handlers are called, the system is dead.
         wdt_reset();
      }
   };
}

 /**@}*/
 /**@}*/
 /**@} ---------------------------  End of file  --------------------------- */