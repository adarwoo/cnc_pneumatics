#ifndef state_machine_h_included
#define state_machine_h_included

#include <boost/sml.hpp>

namespace sml = boost::sml;

// Create those simple events
struct event_open {};
struct event_close {};
struct event_door_is_up {};
struct event_door_is_down {};
struct event_door_moving_up {};
struct event_door_moving_down {};
struct event_timeout {};
   
extern void on_pneumatic_input_change(void *);

namespace valve
{
   // Constants
   constexpr auto moving_timeout = TIMER_SECONDS(3);
   constexpr auto complete_timeout = TIMER_SECONDS(8);

   // Locals
   timer_instance_t timer = TIMER_INVALID_INSTANCE;
   
   // Helpers as lambdas
   auto arm_timer = [](timer_count_t c) {
      timer = timer_arm(react_cmd_timeout, timer_get_count_from_now(c), 0, 0);
   };
   
   /************************************************************************/
   /* Transition lambdas                                                   */
   /************************************************************************/
   
   auto push_on  = [] { 
      on_pneumatic_input_change( pin_and_value_as_arg(IN_DOOR_UP, true) );
      digitial_output_start(led_door_opening, TIMER_SECONDS(1), "+1-", true);
      arm_timer(moving_timeout);
   };
   
   auto push_off = [] { 
      on_pneumatic_input_change( pin_and_value_as_arg(IN_DOOR_UP, false) );
      digitial_output_set(led_door_opening, false);
      timer_cancel(timer);
   };

   auto push_timeout = [] {
      on_pneumatic_input_change( pin_and_value_as_arg(IN_DOOR_UP, false) );
      digitial_output_start(led_door_opening, TIMER_SECONDS(1), "+4-", true);
   };
   
   auto pull_on = [] {
      on_pneumatic_input_change( pin_and_value_as_arg(IN_DOOR_DOWN, true) );
      digitial_output_start(led_door_closing, TIMER_SECONDS(1), "+1-", true);
      arm_timer(moving_timeout);
   };
   
   auto pull_off = [] {
      on_pneumatic_input_change( pin_and_value_as_arg(IN_DOOR_DOWN, false) );
      digitial_output_set(led_door_closing, false);
      timer_cancel(timer);
   };

   auto pull_timeout = [] {
      on_pneumatic_input_change( pin_and_value_as_arg(IN_DOOR_DOWN, false) );
      digitial_output_start(led_door_closing, TIMER_SECONDS(1), "+4-", true);
   };
   
   auto door_moving = [] {
      timer_cancel(timer);
      arm_timer(complete_timeout);
   };
};


struct door_sm
{
   auto operator()() const noexcept
   {
      using namespace sml;

      return make_transition_table(
         *"unknown"_s + event<event_door_is_up>                              = "opened"_s,
          "unknown"_s + event<event_door_is_down>                            = "closed"_s,
          "closed"_s  + event<event_open>             / valve::push_on       = "opening"_s,
          "opened"_s  + event<event_close>            / valve::pull_on       = "closing"_s,
          "opening"_s + event<event_door_moving_up>   / valve::door_moving   = "opening"_s,
          "opening"_s + event<event_timeout>          / valve::push_timeout  = "unknown"_s,
          "opening"_s + event<event_door_is_up>       / valve::push_off      = "opened"_s,
          "closing"_s + event<event_door_moving_down> / valve::door_moving   = "closing"_s,
          "closing"_s + event<event_timeout>          / valve::pull_timeout  = "unknown"_s,
          "closing"_s + event<event_door_is_down>     / valve::pull_off      = "closed"_s
      );
   }
};

#endif  // state_machine_h_included