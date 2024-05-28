#ifndef state_machine_h_included
#define state_machine_h_included

#include <boost/sml.hpp>

namespace sml = boost::sml;

// Create those simple events
struct on_open {};
struct on_close {};
struct door_is_up {};
struct door_is_down {};
struct timeout {};
struct comms_established {};
struct comms_error {};
   
extern void on_pneumatic_input_change(void *);

namespace valve
{
   static constexpr auto push_on  = [] { 
      pin_and_value_t pav{.pin=IN_DOOR_UP}; pav.value=true;
      on_pneumatic_input_change( pav.as_arg ); 
      digitial_output_start(led_door_opening, TIMER_SECONDS(1), "+1-", true);
   };
   
   static constexpr auto push_off = [] { 
      pin_and_value_t pav{.pin=IN_DOOR_UP}; pav.value=false;
      on_pneumatic_input_change( pav.as_arg );
      digitial_output_set(led_door_opening, false);
   };

   static constexpr auto push_timeout = [] {
      pin_and_value_t pav{.pin=IN_DOOR_UP}; pav.value=false;
      on_pneumatic_input_change( pav.as_arg );
      digitial_output_start(led_door_closing, TIMER_SECONDS(1), "+3-", true);
   };
   
   static constexpr auto pull_on  = [] {
      pin_and_value_t pav{.pin=IN_DOOR_DOWN}; pav.value=true;
      on_pneumatic_input_change( pav.as_arg );
      digitial_output_start(led_door_closing, TIMER_SECONDS(1), "+1-", true);
   };
   
   static constexpr auto pull_off = [] {
      pin_and_value_t pav{.pin=IN_DOOR_DOWN}; pav.value=false;
      on_pneumatic_input_change( pav.as_arg );
      digitial_output_set(led_door_closing, false);
   };

   static constexpr auto pull_timeout = [] {
      pin_and_value_t pav{.pin=IN_DOOR_DOWN}; pav.value=false;
      on_pneumatic_input_change( pav.as_arg );
      digitial_output_start(led_door_closing, TIMER_SECONDS(1), "+3-", true);
   };
};


struct door_sm
{
   auto operator()() const noexcept
   {
      using namespace sml;

      return make_transition_table(
         *"unknown"_s + event<door_is_up>                          = "opened"_s,
          "unknown"_s + event<door_is_down>                        = "closed"_s,
          "closed"_s  + event<on_open>      / valve::push_on       = "opening"_s,
          "opened"_s  + event<on_close>     / valve::pull_on       = "closing"_s,
          "opening"_s + event<timeout>      / valve::push_timeout  = "unknown"_s,
          "opening"_s + event<door_is_up>   / valve::push_off      = "opened"_s,
          "closing"_s + event<timeout>      / valve::pull_timeout  = "unknown"_s,
          "closing"_s + event<door_is_down> / valve::pull_off      = "closed"_s
      );
   }
};

#endif  // state_machine_h_included