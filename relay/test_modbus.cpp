#include <cstdint>

// Define the core modbus template
template <typename Board>
struct modbus {
    template <std::uint8_t SlaveAddress, typename... Commands>
    constexpr static auto create_modbus_slave(Commands&&... commands) {
        return ModbusTree<Commands...>{std::forward<Commands>(commands)...};
    }
};

// Structure for a command node
template <typename Matcher, typename Callback>
struct CommandNode {
    Matcher matcher;
    Callback callback;

    constexpr CommandNode(Matcher m, Callback c) : matcher(m), callback(c) {}

    // Evaluates the matcher and executes callback
    template <typename T>
    constexpr void evaluate(T value) const {
        if (matcher(value)) {
            callback();
        }
    }
};

// Structure for modbus command grouping, like read_coils, write_single_coil
template <typename... Nodes>
struct ModbusCommand {
    std::tuple<Nodes...> nodes;

    constexpr ModbusCommand(Nodes&&... n) : nodes(std::forward<Nodes>(n)...) {}

    // Evaluate command by matching the appropriate node
    template <typename T>
    constexpr void evaluate(T value) const {
        std::apply([&](const auto&... node) { ((node.evaluate(value)), ...); }, nodes);
    }
};

// Utility to construct matcher branches
template <typename Matcher, typename... Branches>
constexpr auto match_range(Branches&&... branches) {
    return ModbusCommand<Branches...>{std::forward<Branches>(branches)...};
}

// Match object
template <typename T = uint16_t>
constexpr auto match(T value) {
    return [value](T v) {
        return v == value;
    };
}

// Match range
template <typename T = uint16_t>
constexpr auto match(T start, T end) {
    return [start, end](T v) {
        return v >= start && v <= end;
    };
}

// Function to tie a matcher to a callback
template <typename Matcher, typename Callback>
constexpr auto operator/(Matcher m, Callback c) {
    return CommandNode<Matcher, Callback>{m, c};
}

// Example usage
constexpr auto on_read_coils = []() { /* callback implementation */ };
constexpr auto turn_relay_off = []() { /* callback implementation */ };
constexpr auto turn_relay_on = []() { /* callback implementation */ };
constexpr auto toggle_relay = []() { /* callback implementation */ };
constexpr auto turn_all_relays_off = []() { /* callback implementation */ };
constexpr auto turn_all_relays_on = []() { /* callback implementation */ };
constexpr auto toggle_all_relays = []() { /* callback implementation */ };

// Creating modbus slave with structured matchers and callbacks
constexpr auto modbus_slave = modbus<board::rs485>::create_modbus_slave<0x44>(
    modbus::read_coils(
        match(0, 3) / on_read_coils
    ),
    modbus::write_single_coil(
        match_range(0, 3,
            match_range(
                  match(0) / turn_relay_off,
                  match(255) / turn_relay_on,
                  match(0xAA) / toggle_relay
            ),
        ),
        match(255,
            match(0) / turn_all_relays_off,
            match(255) / turn_all_relays_on,
            match(0xAA) / toggle_all_relays
        )
    ),
    modbus::read_holding_registers(
        match(1) / on_read_version
    ),
    modbus::read_input_registers(
        match(2) / on_read_baud_rate,
        match(3) / on_read_stop_and_data_bits,
        match(5) / on_read_parity_bits,
        match(6) / on_read_response_delay,
        match(7) / on_read_modbus_mode,
        match(9) / on_read_watchdog,
        match<uint32_t>(33) / on_read_received_packets,
        match<uint32_t>(34) / on_read_errored_packets
    )
);

// Creating modbus slave with structured matchers and callbacks
constexpr auto modbus_slave = modbus<board::rs485>::create_modbus_slave<0x44>(
   modbus::read_coils<
      match_range<0, 3>(on_read_coils)
   >,
   modbus::write_single_coil(
      match_range<0, 3,
         match<0, turn_relay_off>,
         match<255 ,turn_relay_on>,
         match<0xAA, toggle_relay>
      >,
      match<255,
         match<0, turn_all_relays_off>
         match<255, turn_all_relays_on>
         match<0xAA, toggle_all_relays>
      >
    >,
    modbus::read_holding_registers<
        match<1, on_read_version>
    >,
    modbus::read_input_registers<
        match<2, on_read_baud_rate>,
        match<3, on_read_stop_and_data_bits>,
        match<5, on_read_parity_bits>,
        match<6, on_read_response_delay>,
        match<7, on_read_modbus_mode>,
        match<9, on_read_watchdog>,
        match<33, on_read_received_packets>,
        match<34, on_read_errored_packets>
    >
);

class Tree
{
   using call_matcher = [](char);
};


constexpr auto modbus_slave = create_tree(
   match_single(1,
      match_range<0, 3>(on_read_coils)
   >,
   match_single<2,
      match_range<0, 3,
         match<0, turn_relay_off>,
         match<255 ,turn_relay_on>,
         match<0xAA, toggle_relay>
      >,
      match<255,
         match<0, turn_all_relays_off>
         match<255, turn_all_relays_on>
         match<0xAA, toggle_all_relays>
      >
    >
);

// Each node yeilds a type which yields an index.
// The nodes are flatten to a variant type
T0 = everything. Within T0, we have a vector of all initial commands
T1 = match_single<1, match_range<0, 3>(LAMBDA on_read_coils)
T2 = match_range<0, 3>(LAMBDA on_read_coils)
T3 = match_single<2, match_range<0, 3, match<0, turn_relay_off>,  match<255 ,turn_relay_on>, match<0xAA, toggle_relay>>, match<255, match<0, turn_all_relays_off> match<255, turn_all_relays_on>         match<0xAA, toggle_all_relays>
T4 = match_range<0, 3, match<0, turn_relay_off>,         match<255 ,turn_relay_on>,         match<0xAA, toggle_relay>
T5 = match<0, turn_relay_off>
T6 = match<255 ,turn_relay_on>
T7 = match<0xAA, toggle_relay>

So the node id is stored by the handler.

** Alternative
Create a vector of vectors etc....
The processor simply stores the current lambda function which takes a char and returns a strcut with the next lamdba.

// Non const expr
constexpr auto modbus_slave = create_tree(
   match_single(1, {
      match_range(0, 3, {on_read_coils})
   }},
   match_single(2, {
      match_range(0, 3), {
         match(0, {turn_relay_off}),
         match(255 ,{turn_relay_on}),
         match(0xAA, {toggle_relay})
      }},
      match<255,
         match<0, turn_all_relays_off>
         match<255, turn_all_relays_on>
         match<0xAA, toggle_all_relays>
      >
    >
);

//
template<Integral T, >

// modbus slave type has a static_array of matcher objects or type?



char c;
for m :in matchers:
  if m.match_byte(c):

struct Command {
    using Handler = modbus_status_t(*)(char);  // Function pointer for handling commands.

    constexpr modbus_status_t match(char c) const {
        return matcher(c);
    }

    Handler matcher;  // Function that matches the current input and decides the command.
};