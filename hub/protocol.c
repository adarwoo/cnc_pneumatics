/*
 * protocol.c
 *
 * Created: 07/05/2024 15:58:32
 *  Author: micro
 */ 
#include <avr/interrupt.h>

#include "reactor.h"
#include "timer.h"
#include "protocol.h"
#include "ioport.h"

#include "conf_board.h"
#include "conf_prio.h"

/************************************************************************/
/* Constants and defines                                                */
/************************************************************************/
#define NO_NEW_COMMAND_GRACE_PERIOD TIMER_MILLISECONDS(1000)
#define CHECK_COMMS_INTERVAL TIMER_SECONDS(1)

/************************************************************************/
/* Local variables                                                      */
/************************************************************************/

// Current command
static opcodes_cmd_t current = opcodes_cmd_idle;

// Set by the timer to indicate a new command can be processed
bool ready_to_accept_new_command = true;

// Number of communications received since last check
volatile uint16_t message_received_counter = 0;

// Reactor for checking the communication
reactor_handle_t react_check_comms;
reactor_handle_t react_accept_comms;


// Called every second to check that commands are being received
// If no commands are received, turn off all valves
static void _on_check_comms(void *arg)
{
   uint16_t counter;
    
   // Atomically get the counter (the interrupt is incrementing it)
   cli();
   counter = message_received_counter;
   sei();
   
   if ( counter == 0 )
   {
      protocol_process(opcodes_cmd_idle);
   }
   
   // Atomically reset the counter and check again in 1 second
   cli();
   message_received_counter = 0;
   sei();
   
   // OK to drift
   timer_arm( react_check_comms, CHECK_COMMS_INTERVAL, 0, 0);
}

void _on_accept_command_again(void *arg)
{
   ready_to_accept_new_command = true;
}

/**
 * Process data received on the communication channel
 * Called inside an interrupt
 * 
 * @return true if the data is valid
 */
bool protocol_process(uint8_t raw_data)
{
   static ioport_pin_t previous_pin = 0;
   ioport_pin_t new_pin = 0;
   opcodes_cmd_t cmd = opcodes_check_cmd_valid(raw_data);
   
   if ( cmd != opcodes_cmd_error )
   {
      ++message_received_counter;
      
      if ( current != cmd && ready_to_accept_new_command)
      {
         // If the command is valid - store it
         switch ( cmd ) {
         case opcodes_cmd_idle:
            ioport_set_pin_level(IOPORT_TOOL_SETTER_AIR_BLAST, false);
            ioport_set_pin_level(IOPORT_CHUCK_CLAMP, false);
            ioport_set_pin_level(IOPORT_SPINDLE_CLEAN, false);
            ioport_set_pin_level(IOPORT_DOOR_PUSH, false);
            ioport_set_pin_level(IOPORT_DOOR_PULL, false);
            
            // Reset previous pin
            previous_pin = 0;
            new_pin = 0;
            break;
         case opcodes_cmd_push_door: 
            new_pin = IOPORT_DOOR_PUSH; break;
         case opcodes_cmd_pull_door: 
            new_pin = IOPORT_DOOR_PULL; break;
         case opcodes_cmd_blast_toolsetter:  
            new_pin = IOPORT_TOOL_SETTER_AIR_BLAST; break;
         case opcodes_cmd_unclamp_chuck:  
            new_pin = IOPORT_CHUCK_CLAMP; break;
         case opcodes_cmd_blast_spindle:  
            new_pin = IOPORT_SPINDLE_CLEAN; break;
         default:
            break;
         }
         
         if ( new_pin != previous_pin && previous_pin != 0 )
         {
            // Release the previous pin
            ioport_set_pin_level(previous_pin, false);
         }
         
         previous_pin = new_pin;
         
         if ( new_pin != 0 )
         {
            ioport_set_pin_level(new_pin, true);
         }
         
         // Update the current command
         current = cmd;
         
         // Do not allow a new command to be accounted for in the next T cycle
         ready_to_accept_new_command = false;
         timer_arm(react_accept_comms, NO_NEW_COMMAND_GRACE_PERIOD, 0, 0);
      }        
   }

   return cmd != opcodes_cmd_error;
}


void protocol_init(void)
{
   react_accept_comms = reactor_register( _on_accept_command_again, PROTOCOL_CMD_PRIO, 1);
   react_check_comms = reactor_register( _on_check_comms, PROTOCOL_CMD_PRIO, 1);

   // Kick start checking for the communication
   timer_arm(react_check_comms, timer_get_count_from_now(TIMER_SECONDS(1)), 0, 0);
}