/*
 * pressure_monitor.c
 *
 * Created: 07/05/2024 16:50:47
 *  Author: micro
 */ 
#include "ioport.h"

#include "pressure_mon.h"
#include "timer.h"
#include "reactor.h"

#include "conf_board.h"
#include "conf_prio.h"

#define MAX_FILTER_RANGE 32
#define SAMPLE_INTERVAL TIMER_MILLISECONDS(10);


// Hold a count for the last measurement out of 10ms
static uint8_t filter_level = 0;

// Don't let the timer drift
timer_count_t timer_count = 0;

// Hold the current filtered state
bool current_filtered_state = false;

// Reactor handler
reactor_handle_t react_on_sample;


// Callback called by the reactor on timer expiry
static void _on_time_to_sample(void *arg)
{
   if ( ioport_get_pin_level(IOPORT_PRESSURE_READOUT) )
   {
      if ( filter_level < MAX_FILTER_RANGE )
      {
         ++filter_level;
      }
      else
      {
         current_filtered_state = true;
      }
      
   }
   else
   {
      if ( filter_level > 0 )
      {
         --filter_level;
      }
      else
      {
         current_filtered_state = false;
      }
   }
 
   // Rearm at steady interval
   timer_count += SAMPLE_INTERVAL
   timer_arm(react_on_sample, timer_count, 0, 0);
}


void pressure_mon_init(void)
{
   react_on_sample = reactor_register(_on_time_to_sample, PRESSURE_MON_PRIO, 1);
   timer_count = timer_get_count_from_now(0);
   
   timer_arm(react_on_sample, timer_count, 0, 0);
}

bool pressure_mon_get_status(void)
{
   return current_filtered_state;
}
