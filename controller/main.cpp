/**
 * @file
 * Main entry point for the pneumatic controller
 * Defines all input and output handling objects and most of the reactors
 * @author gax
 */
#include <limits.h>

#include "status_codes.h"
#include "cpp.h"
#include "sysclk.h"
#include "ioport.h"
#include "reactor.h"
#include "timer.h"
#include "digital_input.h"
#include "digital_output.h"
#include "piezzo.h"
#include "board.h"

#include "op_codes.h"
#include "i2c.h"


/** Number of errors which trigger a shutdown */
static constexpr auto COMMS_TOO_MANY_ERRORS = 10;

/** Duration of the filter (or no re-trigger period) for digital inputs */
static constexpr auto DI_FILT4 = TIMER_MILLISECONDS(40);

/** Time in seconds when communications faults are tolerated */
static constexpr auto COMMS_GRACE_PERIOD = TIMER_SECONDS(5);

/** Time between i2c send */
static constexpr auto I2C_DELAY_BETWEEN_TRANSMIT = TIMER_MILLISECONDS(100);


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
static void on_pneumatic_input_change(void *arg);
static void on_beep_input(void *arg);
static void on_sounder(void *arg);
static void on_send_i2c_command(void *arg);
static void on_i2c_error(void *);
static void on_i2c_read(void *);
static void on_comms_grace_over(void *);
static void on_door_sensor_change(void *);
static void on_door_cmd(void *);
static void on_cmd_timeout(void *);


/************************************************************************/
/* Local variables                                                      */
/************************************************************************/
reactor_handle_t react_input_change =     reactor_register(on_pneumatic_input_change, reactor_prio_medium,         1);
reactor_handle_t react_beep =             reactor_register(on_beep_input,             reactor_prio_high,           1);
reactor_handle_t react_sounder =          reactor_register(on_sounder,                reactor_prio_medium_plus,    1);
reactor_handle_t react_i2c_command =      reactor_register(on_send_i2c_command,       reactor_prio_very_high_plus, 2);
reactor_handle_t react_i2c_error =        reactor_register(on_i2c_error,              reactor_prio_medium_plus,    1);
reactor_handle_t react_i2c_read =         reactor_register(on_i2c_read,               reactor_prio_medium_minus,   1);
reactor_handle_t react_comms_grace_over = reactor_register(on_comms_grace_over,       reactor_prio_low,            1);
reactor_handle_t react_door_sensor =      reactor_register(on_door_sensor_change,     reactor_prio_medium_minus,   1);
reactor_handle_t react_door_cmd =         reactor_register(on_door_cmd,               reactor_prio_low_plus,       1);
reactor_handle_t react_cmd_timeout =      reactor_register(on_cmd_timeout,            reactor_prio_low_plus,       1);

/** Command to send via i2c */
opcodes_cmd_t current_command = opcodes_cmd_idle;

/** Keep track of all the outputs ordered by priority */
output_status_t output_statuses[] = {
    {IN_CHUCK_OPEN, opcodes_cmd_unclamp_chuck, false},
    {IN_SPINDLE_AIR_BLAST, opcodes_cmd_blast_spindle, false},
    {IN_TOOLSET_AIR_BLAST, opcodes_cmd_blast_toolsetter, false},
    {IN_DOOR_UP, opcodes_cmd_pull_door, false},   // Fake input - pretend IN_DOOR_UP is used
    {IN_DOOR_DOWN, opcodes_cmd_push_door, false}, // Same, borrow the sensor input
};

/** Keep a handle for the TWI timer */
timer_instance_t transmit_timer_t;

/** Timer to report for failure of connection */
timer_instance_t connection_check_timer_t;

/** Count the number of transmit errors */
uint8_t comms_error_count = 0;

/** If true, communications error are not fatal */
bool comms_in_error_grace_period_active = true;

/** Flag set to end sending following too many errors */
bool stop_transmit = false;

/** Timer used to transmit over the i2c */
static timer_instance_t transmit_timer = TIMER_INVALID_INSTANCE;


/*
 * Outputs                                                              
 */
digital_output_t led_fault = digital_output(LED_FAULT);
digital_output_t led_chuck = digital_output(LED_CHUCK);
digital_output_t led_door_opening = digital_output(LED_DOOR_OPENING);
digital_output_t led_door_closing = digital_output(LED_DOOR_CLOSING);

// Pressure value from the chuck
digital_output_t chuck_released_oc = digital_output(OC_CHUCK_RELEASED);

/*
 * State machine - include like a .inc
 */
#include "state_machine.hpp"

/** Create the state machine */
boost::sml::sm<door_sm> door_sm;


/************************************************************************/
/* Local functions                                                      */
/************************************************************************/

/** Now lack of connection and transmit errors are accounted for */
static void on_comms_grace_over(void *)
{
   comms_in_error_grace_period_active = false;
}

/** Send an i2c command to the hub */
static void on_send_i2c_command(void *arg)
{
   // If the error count is too high - stop sending
#ifdef NDEBUG
   if ( ! stop_transmit )
#endif
   {   
      // Send an i2c command
      // The i2c should never be busy since we delay the transmit by 5 times a single transmit
      if ( i2c_is_busy() )
      {
	     // Count an error
         on_i2c_error(0);
      }
      else
      {
         i2c_master_send(current_command);
      }
      
      transmit_timer = timer_arm(
         react_i2c_command, timer_get_count_from_now(I2C_DELAY_BETWEEN_TRANSMIT), 0, 0);
   }
}   

/** 
 * (Re)Start periodic transmit to the hub
 * Re-initiate the periodic transmit.
 * Cancel any on-going wait, and transmit right away.
 */
static void trigger_next_transmit(void)
{
   if (transmit_timer != TIMER_INVALID_INSTANCE)
   {
      timer_cancel(transmit_timer);
   }

   // Start transmit in 1ms (at least). A whole frame take 300us.
   // This way, there cannot be a collision with the on-going frame
   transmit_timer = timer_arm(
      react_i2c_command, timer_get_count_from_now(TIMER_MILLISECONDS(1)), 0, 0);
}

/** Check the pneumatic inputs, and let the hub know */
static void refresh_opcode(void)
{
   // If we get to the end of the iteration and none are on, state is idle
   opcodes_cmd_t new_cmd = opcodes_cmd_idle;

   // Update with the highest priority
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
      trigger_next_transmit();
   }
}

/** Called when a timeout is occuring during a door open/close */
static void on_cmd_timeout(void *arg)
{
   door_sm.process_event(event_timeout{});
}

/**
 * Called with the input changes (high to low or low to high)
 * Pass to the state machine to handle the request
 */
static void on_door_cmd(void *arg)
{
   pin_and_value_t pin_and_value { .as_arg = arg };
   
   // Pump the status into the state machine
   if ( pin_and_value.value )
   {
      door_sm.process_event(event_open{});
   }         
   else
   {
      door_sm.process_event(event_close{});
   }         
}

/** Pass the information down when the change input */
static void on_door_sensor_change(void *arg)
{
   pin_and_value_t pin_and_value { .as_arg = arg };
   
   if ( pin_and_value.pin == IN_DOOR_DOWN )
   {
      // For the down sensor, forward to the OC output
      ioport_set_pin_level(OC_DOOR_CLOSED, pin_and_value.value);

      if ( pin_and_value.value )      
      {
         // Let the state machine know
         door_sm.process_event(event_door_is_down{});
      }
      else
      {
         door_sm.process_event(event_door_moving_up{});
      }
   }
   else if ( pin_and_value.pin == IN_DOOR_UP )
   {
      if ( pin_and_value.value )
      {
         // Let the state machine know
         door_sm.process_event(event_door_is_up{});
      }
      else
      {
         door_sm.process_event(event_door_moving_down{});
      }
   }
}

/** A key was pushed - sound it */
static void on_beep_input(void *arg)
{
   piezzo_play(150, "B4");
}

/**
 * The sounder is beeping to warn the door is opening or closing
 * Note: This takes over every other sounds.
 */
static void on_sounder(void *arg)
{
   pin_and_value_t pin_and_value;
   pin_and_value.as_arg = arg;

   if (pin_and_value.value)
   {
      // Play long continuous tone
      piezzo_start_tone(PIEZZO_FREQ_TO_PWM(1400), 0);
   }
   else
   {
      piezzo_stop_tone();
   }
}


/** 
 * Any of the pneumatic control inputs changed
 * Let the i2c master forward the information to the hub 
 * Note : Only 1 output is allowed 'on' at a time. The table output_statuses
 * list them in priority order - so, only 1 value control is passed out.
 */
static void on_pneumatic_input_change(void *arg)
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
   
   // Drive the chuck LED
   if (pav.pin == IN_CHUCK_OPEN)
   {
      digitial_output_set(led_chuck, pav.value);
   }
}


/**
 * Called on error detected on the i2c
 * The slave should be up at the same time as the master.
 * The system will stop if the slave fails to answer or if too
 *  many consecutive errors are detected.
 * The comms_error_count is incremented by 2 on error, and goes down by 1 if
 *  a good communication is detected.
 * Sound the alert, and go into failsafe mode.
 */
static void on_i2c_error(void *arg)
{
   // Check if the system hit a no-recovery
   if ( ! comms_in_error_grace_period_active )
   {
      // Increment errors at the twice the rate of good packets
      comms_error_count += 2;
   }      
      
   if ( comms_error_count >= COMMS_TOO_MANY_ERRORS )
   {
      // Turn off the pressure input as a fail safe
      digitial_output_set(chuck_released_oc, false);
         
      // Light the communication error LED
      digitial_output_set(led_fault, true);

      // In release, turn of communication and sound the beeper to signal an error
#ifdef NDEBUG
      piezzo_start_tone(PIEZZO_FREQ_TO_PWM(2000), TIMER_SECONDS(5));

      // Flag end of transmit
      stop_transmit = true;
#endif
   }
   else
   {
      // Flash the led once. For repeated errors, it will light
      digitial_output_start(led_fault, TIMER_MILLISECONDS(50), "+-", false);
   }
}

/**
 * Handle a successful read from the i2c slave
 * The returned value was also checked for errors
 */
static void on_i2c_read(void *arg)
{
   bool status = (bool)arg;

   // Decrement the error count
   // We need 2 good Transmit for one Receive
   if ( comms_error_count > 0 )
   {
      --comms_error_count;
   }

   // Re-inject the pressure back to Masso
   digitial_output_set( chuck_released_oc, status );
}


int main(void)
{
   board_init();
   
   constexpr auto input = [](ioport_pin_t p, reactor_handle_t h) {
      return digital_input(p, h, IOPORT_SENSE_DISABLE, DI_FILT4 );
   };

   input(IN_DOOR_UP,           react_door_sensor );
   input(IN_DOOR_DOWN,         react_door_sensor );
   input(IN_CHUCK_OPEN,        react_input_change);
   input(IN_SPINDLE_AIR_BLAST, react_input_change);
   input(IN_TOOLSET_AIR_BLAST, react_input_change);
   input(IN_DOOR_OPEN_CLOSE,   react_door_cmd    );
   input(IN_SOUNDER,           react_sounder     );
   input(IN_BEEP,              react_beep        );

   // Flash all LEDs for 2 second to start with to check none are defective
   digitial_output_start(led_fault,        1000, "++-", false);
   digitial_output_start(led_chuck,        1000, "++-", false);
   digitial_output_start(led_door_opening, 1000, "++-", false);
   digitial_output_start(led_door_closing, 1000, "++-", false);

   // Register for i2c events
   i2c_init(react_i2c_read, react_i2c_error);

   // Start sending to the i2c periodically
   trigger_next_transmit();
   
   // Start a timer to tolerate communications error for the first N seconds
   timer_arm(
	  react_comms_grace_over, 
	  timer_get_count_from_now(COMMS_GRACE_PERIOD),
	  0, 0
   );

   // Play moon_cresta tune
   #ifndef DEBUG
   piezzo_play(190, "C,3 R C E G E G E D R D F A2~A3 B G E B G E B G E C' R B, C'~C1");
   #endif
   
   // Run the reactor
   reactor_run();
}
