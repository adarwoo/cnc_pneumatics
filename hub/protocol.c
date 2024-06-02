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

/** The current command */
static opcodes_cmd_t _current_cmd = opcodes_cmd_idle;

/** If true, accept a new command */
bool _ready_to_accept_new_command = true;

/** Instance of the timer accepting command. We need to cancel this timer */
timer_instance_t _ready_to_accept_timer_instance = TIMER_INVALID_INSTANCE;

/** Number of communications received since last check */
volatile uint16_t _message_received_counter = 0;

/** Reactor for checking the communication */
static reactor_handle_t _react_check_comms;

/** Reactor for setting the _ready_to_accept_new_command */
static reactor_handle_t _react_accept_comms;


/************************************************************************/
/* Local functions                                                      */
/************************************************************************/

/** 
 * Apply the given command without filter
 *
 * Make sure to turn off all unused, and on the one valve
 * This function is responsible for updating the variable _ready_to_accept_new_command
 *  which other functions can check.
 * A timer is armed to manage this variable to avoid fast switching of the valves.
 *
 * @param cmd The command to honor
 */
static void _protocol_process(opcodes_cmd_t cmd)
{
   // Turn off all valves
   ioport_set_pin_level(IOPORT_TOOL_SETTER_AIR_BLAST, false);
   ioport_set_pin_level(IOPORT_CHUCK_CLAMP, false);
   ioport_set_pin_level(IOPORT_SPINDLE_CLEAN, false);
   ioport_set_pin_level(IOPORT_DOOR_PUSH, false);
   ioport_set_pin_level(IOPORT_DOOR_PULL, false);

   // If the command is valid - store it
   switch ( cmd )
   {
   case opcodes_cmd_idle:
      break;
   case opcodes_cmd_push_door:
      ioport_set_pin_level(IOPORT_DOOR_PUSH, true);break;
   case opcodes_cmd_pull_door:
      ioport_set_pin_level(IOPORT_DOOR_PULL, true); break;
   case opcodes_cmd_blast_toolsetter:
      ioport_set_pin_level(IOPORT_TOOL_SETTER_AIR_BLAST, true); break;
   case opcodes_cmd_unclamp_chuck:
      ioport_set_pin_level(IOPORT_CHUCK_CLAMP, true); break;
   case opcodes_cmd_blast_spindle:
      ioport_set_pin_level(IOPORT_SPINDLE_CLEAN, true); break;
   default:
      break;
   }
   
   // Do not allow a new command to be accounted for in the next T cycle
   _ready_to_accept_new_command = false;
   
   // If a timer is already running, cancel it
   if ( _ready_to_accept_timer_instance != TIMER_INVALID_INSTANCE )
   {
      timer_cancel(_ready_to_accept_timer_instance);
   }
   
   // Start a new timer 
   _ready_to_accept_timer_instance = timer_arm(
      _react_accept_comms,
      timer_get_count_from_now(NO_NEW_COMMAND_GRACE_PERIOD),
      0, 0
   );
}

/**
 * Called every second to check that commands are being received.
 * If no commands are received, turn off all valves.
 */
static void _on_check_comms(void *arg)
{
   if ( _message_received_counter == 0 )
   {
      // Reset all valves
      _protocol_process(opcodes_cmd_idle);
      
      // Assume the system is idle
      _current_cmd = opcodes_cmd_idle;
   }
   
   _message_received_counter = 0;
}

/**
 * Called by the timeout reactor to allow processing incoming commands again
 */
static void _on_accept_command_again(void *arg)
{
   _ready_to_accept_new_command = true;
   
   // Mark as unused
   _ready_to_accept_timer_instance = TIMER_INVALID_INSTANCE;
}

/************************************************************************/
/* Public functions                                                     */
/************************************************************************/

/**
 * Handle a cmd received on the i2c.
 * These come in at 10 a seconds.
 * Check validity, only handle change.
 */
void protocol_handle_traffic(void *arg)
{
   opcodes_cmd_t cmd = (opcodes_cmd_t)arg;
   
   // Make sure the value is valid
   if ( opcodes_check_cmd_valid(cmd) )
   {
      // The increase the counter, make sure we are receiving and the content is valid
      ++_message_received_counter;
      
      if ( _current_cmd != cmd && _ready_to_accept_new_command )
      {
         _current_cmd = cmd;
         
         _protocol_process(cmd);
      } 
   }
}


void protocol_init(void)
{
   _react_accept_comms = reactor_register( _on_accept_command_again, PROTOCOL_CMD_PRIO, 1);
   _react_check_comms = reactor_register( _on_check_comms, PROTOCOL_CMD_PRIO, 1);

   // Kick start checking for the communication
   timer_arm(
      _react_check_comms, 
      timer_get_count_from_now(TIMER_SECONDS(5)),
      TIMER_SECONDS(2), // Repeat every 2 seconds
      0
   );
}