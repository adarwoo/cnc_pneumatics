/*
 * main.cpp
 *
 * Created: 03/05/2024 10:33:54
 * Author : micro
 */
#include "status_codes.h"
#include "cpp.h"
#include "sysclk.h"
#include "ioport.h"
#include "reactor.h"
#include "timer.h"
#include "digital_input.h"
#include "digital_output.h"
#include "piezzo.h"

#include "op_codes.h"
#include "i2c.h"

extern "C" void board_init(void);

/** Number of errors which trigger a shutdown */
#define COMMS_TOO_MANY_ERRORS 10

/************************************************************************/
/* Local types                                                          */
/************************************************************************/
typedef struct
{
   ioport_pin_t pin;     ///< Input pin
   opcodes_cmd_t opcode; ///< Matching opcode
   bool state;           ///< Last seen state of this output
} output_status_t;


/************************************************************************/
/* Forward declarations and event Handlers                              */
/************************************************************************/
static void on_door_down_input_change(void *arg);
static void on_pneumatic_input_change(void *arg);
static void on_beep_input(void *arg);
static void on_sounder(void *arg);
static void on_send_i2c_command(void *arg);
static void on_i2c_error(void *);
static void on_i2c_read(void *);
static void on_connection_failed_to_establish(void *);
static void on_sm_event(void *);


/************************************************************************/
/* Local variables                                                      */
/************************************************************************/
reactor_handle_t react_door_input =
    reactor_register(on_door_down_input_change,
    reactor_prio_high_minus, 1);

reactor_handle_t react_input_change =
    reactor_register(on_pneumatic_input_change,
    reactor_prio_medium, 1);

reactor_handle_t react_beep =
    reactor_register(on_beep_input,
    reactor_prio_high, 1);

reactor_handle_t react_sounder =
    reactor_register(on_sounder,
    reactor_prio_medium_plus, 1);

reactor_handle_t react_i2c_command =
    reactor_register(on_send_i2c_command,
    reactor_prio_very_high_plus, 1);

reactor_handle_t react_i2c_error =
    reactor_register(on_i2c_error,
    reactor_prio_medium_plus, 1);

reactor_handle_t react_i2c_read =
    reactor_register(on_i2c_read,
    reactor_prio_medium_minus, 1);

reactor_handle_t react_sm_event =
    reactor_register(on_sm_event,
    reactor_prio_medium, 1);

/** Command to send via i2c */
opcodes_cmd_t current_command = opcodes_cmd_idle;

/** Keep track of all the outputs ordered by priority */
output_status_t output_statuses[] = {
    {IN_CHUCK_OPEN, opcodes_cmd_unclamp_chuck, false},
    {IN_SPINDLE_AIR_BLAST, opcodes_cmd_blast_spindle, false},
    {IN_TOOLSET_AIR_BLAST, opcodes_cmd_blast_toolsetter, false},
    {0, opcodes_cmd_pull_door, false},
    {0, opcodes_cmd_push_door, false},
};

/** Keep a handle for the TWI timer */
timer_instance_t transmit_timer_t;

/** Timer to report for failure of connection */
timer_instance_t connection_check_timer_t;

/** Count the number of transmit errors */
uint8_t comms_error_count = 0;

/** Sounding door alarm */
bool sound_door_alarm = false;

/************************************************************************/
/* Outputs                                                              */
/************************************************************************/
digital_output_t led_fault = digital_output(LED_FAULT);
digital_output_t led_chuck = digital_output(LED_CHUCK);
digital_output_t led_door_opening = digital_output(LED_DOOR_OPENING);
digital_output_t led_door_closing = digital_output(LED_DOOR_CLOSING);

// Pressure value from the chuck
digital_output_t chuck_released_oc = digital_output(OC_CHUCH_RELEASED);


/************************************************************************/
/* Local functions                                                      */
/************************************************************************/

/** Send an i2c command to the hub */
void on_send_i2c_command(void *arg)
{
   // Send an i2c command
   // The i2c should never be busy since we delay the transmit by 5 times a single transmit
   if ( ! i2c_is_busy() )
   {
      i2c_master_send(current_command);
   }
}

/** Start periodic transmit to the hub */
void start_periodic_transmit(bool start)
{
   static timer_instance_t transmit_timer_t = TIMER_INVALID_INSTANCE;

   if (transmit_timer_t != TIMER_INVALID_INSTANCE)
   {
      timer_cancel(transmit_timer_t);
   }

   if (start)
   {
      transmit_timer_t = timer_arm(
          react_i2c_command,
          timer_get_count_from_now(TIMER_MILLISECONDS(1)),
          TIMER_MILLISECONDS(100),
          (void *)current_command);
   }
}

/** Check the pneumatic inputs, and let the hub know */
void refresh_opcode(void)
{
   // If we get to the end of the iteration and none are on, state is idle
   opcodes_cmd_t new_cmd = opcodes_cmd_idle;

   // Update with the highest prio
   for (uint8_t i = 0; i < COUNTOF(output_statuses); ++i)
   {
      if (output_statuses[i].state)
      {
         new_cmd = output_statuses[i].opcode;
         break;
      }
   }

   // Has the state changes (very unlikely it has not)
   if (current_command != new_cmd)
   {
      current_command = new_cmd;

      // Cancel the TWI timer to transmit here and now
      start_periodic_transmit(true);
   }
}

/**
 * Called by the timer to indicate a connection failed
 * Stop all transmittion and sound the alarm.
 */
void on_connection_failed_to_establish(void)
{
   start_periodic_transmit(false);
}

/** Pass the information down when the change input */
void on_door_down_input_change(void *arg)
{
   pin_and_value_t pin_and_value;
   pin_and_value.as_arg = arg;

   ioport_set_pin_level(OC_DOOR_CLOSED, pin_and_value.value);
}

/** A key was pushed - sound it */
void on_beep_input(void *arg)
{
   // Only play if the door alarm is off
   if ( ! sound_door_alarm )
   {
      piezzo_start_tone(PIEZZO_FREQ_TO_PWM(1400), TIMER_MILLISECONDS(200));
   }
}

/**
 * The sounder is beeping 
 * Note: st
 */
void on_sounder(void *arg)
{
   pin_and_value_t pin_and_value;
   pin_and_value.as_arg = arg;

   if (pin_and_value.value)
   {
      // Play long continuous tone
      piezzo_start_tone(PIEZZO_FREQ_TO_PWM(800), 0);
      
      sound_door_alarm = true;
   }
   else
   {
      piezzo_stop_tone();

      sound_door_alarm = false;
   }
}

void on_door_status_change(void *arg)
{
   // Grab the value of the pin
   pin_and_value_t pav;
   
   pav.as_arg = arg;

   // Pump into the state machine
   if (pav.value)
   {
      // process_event(sm, door_opening{});
   }
   else
   {
      // process_event(sm, door_closing{});
   }
}

/** Input has change, let the i2c master forward the information to the hub */
void on_pneumatic_input_change(void *arg)
{
   // Grab the pin that changed
   pin_and_value_t pav;
   pav.as_arg = arg;

   // Iterate
   for (uint8_t i = 0; i < COUNTOF(output_statuses); ++i)
   {
      if (output_statuses[i].pin == pav.pin)
      {
         output_statuses[i].state = pav.value;
         refresh_opcode();
         break;
      }
   }
}

void on_sm_event(void *arg)
{
   // TODO -> Inject into SM
}

void on_i2c_error(void *arg)
{
   status_code_t status = (status_code_t)((uint16_t)arg);

   comms_error_count += 2;

   if ( comms_error_count > COMMS_TOO_MANY_ERRORS )
   {
      digitial_output_start(led_fault, TIMER_MILLISECONDS(500), "+-+-+-+-", true);
      piezzo_start_tone(PIEZZO_FREQ_TO_PWM(2000), TIMER_SECONDS(5));
   }
   else
   {
      // Signal the error on the LED
      digitial_output_start(led_fault, TIMER_MILLISECONDS(500), "+-+-+-+-", true);
   }
}

/**
 * Handle a sucessful read from the i2c slave
 * The returned value was also checked for errors
 */
void on_i2c_read(void *arg)
{
   bool status = (bool)arg;

   // Decrement the error count. We need 2 good Tx for one Rx
   if ( comms_error_count )
   {
      --comms_error_count;
   }

   // Re-inject the pressure back to Masso
   digitial_output_set( chuck_released_oc, status );
}


int main(void)
{
   board_init();

   digital_input(IN_CHUCK_OPEN, react_input_change, 0, 4);
   digital_input(IN_SPINDLE_AIR_BLAST, react_input_change, 0, 4);
   digital_input(IN_TOOLSET_AIR_BLAST, react_input_change, 0, 4);
   digital_input(IN_DOOR_OPEN_CLOSE, react_sm_event, 0, 4);
   digital_input(IN_DOOR_UP, react_sm_event, 0, 4);
   digital_input(IN_DOOR_DOWN, react_sm_event, 0, 4);

   digital_input(IN_SOUNDER, react_sounder, 0, 4);

   digital_input(IN_BEEP, react_beep, IOPORT_SENSE_RISING, 10);

   // Flash all LEDs for 2 second to start with to check none are defective
   digitial_output_start(led_fault, 1000, "+1-", true);
   digitial_output_start(led_chuck, 1000, "++-", false);
   digitial_output_start(led_door_opening, 1000, "++-", false);
   digitial_output_start(led_door_closing, 1000, "++-", false);

   // Register for i2c events
   i2c_init(react_i2c_read, react_i2c_error);

   // Start sending to the i2c periodically
   start_periodic_transmit(true);

   // Play moon_cresta tune
   // piezzo_play(190, "C,3 R C E G E G E D R D F A2~A3 B G E B G E B G E C' R B, C'~C1");
   reactor_run();
}
