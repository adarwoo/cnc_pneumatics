#include <boost/sml/sml.hpp>

using namespace sml; // Postfix Notation

struct one_and_a_half_character_timeout {};
struct t35_timeout {};
struct demand_of_emission {}
struct char_received {}

make_transition_table(
 *"intial_state"_s + event<my_event> [ guard ] / action = "idle"_s
, "idle"_s + event<char_received> / reset_timeouts = "reception"_s
, "idle"_s + event<demand_of_emission> = "emission"_s
, "reception"_s + event<char_received> / reset_timeouts = "reception"_s
, "reception"_s + event<char_received> / reset_timeouts = "control_and_waiting"_s
, "control_and_waiting"_s + event<t35_timeout> / reset_timeouts = "idle"_s
, "emission"_s + event<char_sent> [ more_chars] = "emission"_s
, "emission"_s + event<t35_timeout> [ more_chars] = "emission"_s
);