#!/usr/bin/env python3
"""
Define the slave commands

The modbus dictionary take 2 kw callbacks and devices
The device key takes an address give following @ symbol writen in decimal or hexadecimal with 0x preceding

The callbacks is a dictionary of callback function name and their list of arguments.
Each argument must be of any of the following type:
  u8, u16, u32 : Unsigned integer of the given length
  s8, s16, s32 : Signed
  f32 : Floats
A tuple of type and variable name can be provided.

The devices part is a list of modbus commands expressed as tuples.
The tuple is made of:
  A modbus RTU type command. Must be one of the following:
        read_coils (0x01)
        read_discrete_inputs     (0x02)
        read_holding_registers   (0x03)
        read_input_registers     (0x04)
        write_single_coil	     (0x05)
        write_single_register    (0x06)
        write_multiple_coils     (0x0f)
        write_multiple_registers (0x10)
        read_write_multiple_registers  (0x17)

  Then an number of constructed type object. Each object can take the following arguments:
     A single value
     A range (2 values, from inclusive, to exclusive)
     A list (possible values)
     None - to allow any value

   Warning: As the tree is constructed, each command must have a unique path. Overlapping path will
    generate a compile error.

   Finally, the name of the callback. This name must exists in the callbacks section.
   The parameters are converted to the host reprentation and cast.
   The size is checked during cast. A range 0-0x200 cannot be cast to an 8-bit.
"""
import re
import textwrap

TEMPLATE_CODE="""/**
 * This file was generated to create a state machine for processing
 * uart data used for a modbus RTU. It should be included by
 * the modbus_rtu_slave.cpp file only which will create a full rtu slave device.
 */
#include <stdint.h>

namespace modbus {
    namespace slave {
        enum class process_outcome_t : uint8_t {
            ignore,             // Wait for the stream to stop (3.5T) as it is not for us
            expecting_more,     // Char was processed, another is expected
            invalid_value,      // Value is out-of-range or not valid
            unsupported_operation
        };

        enum class callback_outcome_t : uint8_t {
            reply_ready,            // A reply is ready to send
            unsupported_operation,  // Operation is not supported
            invalid_value,          // Value is out-of-range or not valid
        };

        // All callbacks registered
        @PROTOTYPES@

        // All states to consider
        enum class state_t : uint8_t {
            @ENUMS@
        };

        struct Processor {
            state_t state;
            uint8_t idx;
            uint16_t crc;
            bool expecting_crc;

            uint8_t buffer[@BUFSIZE@];

            void reset() {
                state = state_t::DEVICE_ADDRESS;
                idx = 0;
                crc = 0xffff;
                expecting_crc = false;
            }

            Processor() {
                reset();
            }

            inline void update_crc(uint8_t byte) {
                crc = crc ^ byte;

                for (unsigned char j = 1; j <= 8; ++j)
                {
                    bool flag = crc & 0x0001;

                    crc >>=1;

                    if (flag)
                    {
                        crc ^= 0xa001;
                    }
                }
            }

            auto process(const uint8_t c) -> process_outcome_t {
                buffer[idx++] = c; // Store the data

                if ( not expecting_crc ) {
                    update_crc(c); // Update the CRC as we go
                }

                switch(state) {
                @CASES@
                default:
                    break;
                }

                return process_outcome_t::invalid_value;
            }

            /** Called when a T3.5 has been detected, in a good sequence */
            auto process_end_of_frame() -> callback_outcome_t {
                switch(state) {
                @CALLBACKS@
                default:
                    break;
                }

                // This is un-reachable!
                return callback_outcome_t::unsupported_operation;
            }
        }; // struct Processor

    } // namespace slave
} // namespace modbus"""

# Regex to check the device address (and extract it)
DEVICE_ADDR_RE = re.compile(r'device@(?:0x)?([0-9a-fA-F]+)')

# Regex pattern for a valid C function name
VALID_C_FUNCTION_NAME = re.compile(r'^[a-zA-Z_][a-zA-Z0-9_]*$')

# Indent by
INDENT = " " * 4

class Integral:
    """Base class for integral types."""
    def __init__(self, i):
        self.value = i

    @property
    def size(self):
        """Return the size in bytes of the integral type."""
        return self.bits // 8

class _8bits(Integral):
    bits = 8
    ctype = "uint8_t"

class _16bits(Integral):
    bits = 16
    ctype = "uint16_t"

class _32bits(Integral):
    bits = 32
    ctype = "uint32_t"

class Matcher:
    """Base Matcher class for different integral types."""

    def __init__(self, *args, **kwargs):
        if len(args) == 0 or (len(args) == 1 and args[0] is None):
            self.value = None
        elif len(args) == 1:
            value = args[0]

            if isinstance(value, list):
                for _value in value:
                    self.check(_value)
                self.value = value # Assign the list
            else:
                self.value = self.cast(args[0])
        elif len(args) == 2:
            self.value = Range(self.cast(args[0]), self.cast(args[1]))
        else:
            raise ValueError(f"Invalid arguments for {self.__class__.__name__}")

        if "alias" in kwargs:
            self.alias = kwargs["alias"]
        else:
            self.alias = None

        self.pos = None # To be set later

    def cast(self, value):
        """Check and return the value if valid, otherwise raise an error."""
        if self.check(value):
            return value
        raise ValueError(f"Cannot cast the value {value}")

    def check(self, value):
        """Check if the value is valid (to be implemented in subclasses)."""
        raise NotImplementedError

    def fits(self, item):
        """ @return False if the item size is fitting with the given matcher """
        v = item(0)

        # Is it big enough!
        if v.size >= self.size:
            return True

        # 2 cases left 8 for a 16
        if isinstance(self.value, Range):
            # For the range, we need to check if the min and max are fitting
            return v.min <= self.value._from and v.max >= self.value._to
        elif isinstance(self.value, list):
            # Make sure all values for within the min-max range
            for t in self.value:
                if t < v.min or t > v.max:
                    return False

        return True

    def __repr__(self):
        return f"{self.ctype}({self.value})"

    def to_code(self):
        if isinstance(self.value, Range):
            if self.value._from == 0 and isinstance(self, UnsignedMatcher):
                return f"c < {self.value._to}"
            return f"c >= {self.value._from} and c < {self.value._to}"
        elif isinstance(self.value, list):
            return " || ".join(f"c == {value}" for value in self.value)
        elif self.value == None:
            "true"
        else:
            return f"c == {self.value}"

class Range:
    """Simple Range class to hold value ranges."""
    def __init__(self, from_value, to_value):
        self._from = from_value
        self._to = to_value
    def __repr__(self):
        return f"[{self._from}-{self._to}]"

class UnsignedMatcher(Matcher):
    """Matcher for unsigned integral types."""
    def check(self, value):
        return isinstance(value, int) and (0 <= value < (1 << self.bits))
    @property
    def max(self):
        return (1 << self.bits) - 1
    @property
    def min(self):
        return 0

class SignedMatcher(Matcher):
    """Matcher for signed integral types."""
    def check(self, value):
        return isinstance(value, int) and (-(1 << (self.bits - 1)) <= value < (1 << (self.bits - 1)))
    @property
    def max(self):
        return (1 << (self.bits - 1)) - 1
    @property
    def min(self):
        return -(1 << (self.bits - 1))

# Concrete Matcher classes for various types
class u8(UnsignedMatcher, _8bits): pass
class u16(UnsignedMatcher, _16bits): pass
class u32(UnsignedMatcher, _32bits): pass
class s8(SignedMatcher, _8bits): pass
class s16(SignedMatcher, _16bits): pass
class s32(SignedMatcher, _32bits): pass
class f32(Matcher, _32bits):
    def check(self, value):
        return isinstance(value, float)
class Crc(UnsignedMatcher, _16bits):
    _bits = -16 # Negative for little endian
    def to_code(self):
        return "crc == c"

READ_COILS                    = u8(0x01, alias="READ_COILS")
READ_DISCRETE_INPUTS          = u8(0x02, alias="READ_DISCRETE_INPUTS")
READ_HOLDING_REGISTERS        = u8(0x03, alias="READ_HOLDING_REGISTERS")
READ_INPUT_REGISTERS          = u8(0x04, alias="READ_INPUT_REGISTERS")
WRITE_SINGLE_COIL             = u8(0x05, alias="WRITE_SINGLE_COIL")
WRITE_SINGLE_REGISTER         = u8(0x06, alias="WRITE_SINGLE_REGISTER")
WRITE_MULTIPLE_COILS          = u8(0x0F, alias="WRITE_MULTIPLE_COILS")
WRITE_MULTIPLE_REGISTERS      = u8(0x10, alias="WRITE_MULTIPLE_REGISTERS")
READ_WRITE_MULTIPLE_REGISTERS = u8(0x17, alias="READ_WRITE_MULTIPLE_REGISTERS")


class Transition:
    """ Represents a test which triggers a callback or a transition """
    def __init__(self, matcher, next_state):
        self.matcher = matcher
        self.next = next_state
        self.set_crc = False

    def is_crc(self):
        return self.next is not None and isinstance(self.next.ops, Operation)

    def to_code(self, indent):
        tab = INDENT * indent
        opening = close = ""

        opening += f"if ( {self.matcher.to_code()} ) {{\n{tab}"

        if self.set_crc:
            opening += f"{INDENT}expecting_crc = true;\n{tab}"

        close = f"\n{tab}}}"

        return f"{opening}{INDENT}state = state_t::{self.next.name};{close}"

class TransitionGroup:
    """ Holds a group of matchers of the same type """
    def __init__(self, integral, pos):
        self.integral = integral
        self.pos = pos
        self.transitions = []

    def to_code(self, indent):
        tab = INDENT * indent
        retval = str()
        next_flag = False
        size = self.integral.size
        extra_indent = 1 if size > 1 else 0
        extra = INDENT * extra_indent

        crc = isinstance(self.integral, Crc)

        # Redefine c
        if size == 2:
            if crc:
                retval += f"{tab}{extra}uint8_t *data = &buffer[idx-2];\n"
                retval += f"{tab}{extra}{self.integral.ctype} c = (data[1] << 8) | data[0];\n\n"
            else:
                retval += f"{tab}{extra}uint8_t *data = &buffer[idx-2];\n"
                retval += f"{tab}{extra}{self.integral.ctype} c = (data[0] << 8) | data[1];\n\n"
        elif size == 4:
            retval += f"{tab}{extra}uint8_t *data = &buffer[idx-4];\n"
            retval += f"{tab}{extra}{self.integral.ctype} c = data[0] << 24 | data[0] << 16 | data[0] << 8 | data[1];\n\n"

        for matcher in self.transitions:
            if next_flag:
                retval += " else "
            else:
                retval += tab+extra

            next_flag = True
            retval += matcher.to_code(indent+extra_indent)

        retval += f" else {{\n{tab}{extra}{INDENT}return "

        if self.pos == 0:
            retval += "process_outcome_t::ignore"
        elif self.pos == 1:
            retval += "process_outcome_t::unsupported_operation"
        else:
            retval += "process_outcome_t::invalid_value"

        if size == 1:
            return retval

        return f"{tab}if ( idx == {self.pos+size} ) {{\n{retval};\n{INDENT}{tab}}}"

class Operation:
    def __init__(self, name, prototype, chain):
        self.name = name
        self.prototype = prototype
        self.chain = chain

    def to_code(self):
        # Check the prototype to see if we need to pass the buffer data
        # Create from the end
        values_str = []
        chain = [] + self.chain # Force a deep copy
        nargs = len(self.prototype)

        for pos, param in enumerate(reversed(self.prototype)):
            if type(param) == tuple:
                # Skip the name
                param, param_name = param
                param_name = f"'{param_name}'"
            else:
                param_name = f"argument at position {nargs - pos}"

            chain_item = chain.pop() # Pop the last element (and remove from length computation)
            offset = sum(item.size for item in chain if isinstance(item, Integral))

            param_size = param(0).size

            # Make sure the size if compatible
            if not chain_item.fits(param):
                raise ParsingException(f"Cannot fit {chain_item} into {param_name} of type {param.ctype} in {self.name}")

            # Compute offset for casts
            offset += chain_item.size - param_size

            if param_size == 1:
                values_str.insert(0, f"{param.ctype}{{buffer[{offset}]}}")
            elif param_size == 2:
                values_str.insert(0, f"{param.ctype}{{buffer[{offset}]<<8 || buffer[{offset+1}]}}")
            elif param_size == 4:
                values_str.insert(0, f"{param.ctype}{{buffer[{offset}]<<24 || buffer[{offset+1}]<<16 || buffer[{offset+2}]<<8 || buffer[{offset+3}]}}")
            elif param_size == -2: # CRC
                values_str.insert(0, f"{param.ctype}{{buffer[{offset+1}]<<8 || buffer[{offset}]}}")

        # Add the params (the list is ordered)
        return f"{self.name}({', '.join(values_str)});"

class State:
    """ State in the processing of incomming bytes """
    def __init__(self, name, pos=0):
        self.name = name
        self.transition = []
        self.pos = pos

    def add(self, transition):
        """ Append a new transition to the current state """
        self.transition.append(transition)

    def next(self, alias):
        """ @return the name of the next state """
        alias = alias if alias else str(len(self.transition)+1)
        return self.name + "_" + alias

    def __add__(self, suffix):
        return State(self.name + "_" + suffix, self.pos+1)

    def is_final(self):
        return len(self.transition) and isinstance(Operation, self.transition[0])

    def has(self, matcher):
        for transition in self.transition:
            if transition.matcher == matcher:
                return True
        return False

    def get_next_state_of(self, matcher):
        """ Given a matcher, return its transitioning state """
        for transition in self.transition:
            if transition.matcher == matcher:
                return transition.next
        assert(False)

    def to_code_case(self, indent):
        return f"{INDENT * indent}case state_t::{self.name}:\n"

    def to_code(self, indent):
        # Group the transitions into groups by type
        transition_groups = {}
        tab = INDENT * indent
        retval = str()

        for transition in self.transition:
            group = transition_groups.setdefault(
                transition.matcher.__class__,
                TransitionGroup(transition.matcher, self.pos)
            )

            group.transitions.append(transition)

        for tg in transition_groups.values():
            retval += tg.to_code(indent+1)

        return retval + f";\n{tab}{INDENT}}}\n{tab}{INDENT}break;\n"

class OperationState(State):
    def __init__(self, op, name, pos=0):
        super().__init__(name, pos)
        self.op = op

    """ A state which leads to an operation """
    def to_code(self, indent):
        tab = INDENT * indent
        return f"{tab}return {self.op.to_code()};\n{tab}break;\n"

class ParsingException(Exception):
    pass

class CodeGenerator:
    """ Creates the C++ code to parse the modbus data """
    def __init__(self, tree):
        self.counter = 0
        self.states = []
        self.callbacks = {}
        # Compute the maximum message size
        self.max_buf_size = 0
        # Buffer index
        self.buffer_index = 0

        if "callbacks" not in tree:
            raise ParsingException("Callbacks are required")

        self.process_callbacks(tree["callbacks"])

        for key, value in tree.items():
            if key.startswith("device"):
                for cmd in value:
                    self.max_buf_size = max(self.max_buf_size,
                        sum(item.size for item in cmd if isinstance(item, Integral))
                    )

        # Add space for the device address, the command and the CRC
        self.max_buf_size += 4

        self.process_devices(tree)

    def new_state(self, new_state_name, pos):
        """ Add a new state transition """
        # Make sure the name is unique
        names = {state.name for state in self.states if state.name.startswith(new_state_name)}

        count = 1
        alt_name = new_state_name
        while alt_name in names:
            alt_name = new_state_name + "_" + str(count)
            count+=1

        new_state = State(alt_name, pos)
        self.states.append(new_state)
        return new_state

    def generate_code(self):
        placeholders = {
            "BUFSIZE" : str(self.max_buf_size),
            "ENUMS" : self.get_enums_text(3),
            "CASES" : self.get_cases_text(4),
            "CALLBACKS" : self.get_callbacks_text(3),
            "PROTOTYPES" : self.get_prototypes(2),
        }

        # Function to replace each placeholder
        def replace_placeholder(match):
            placeholder = match.group(2)
            endl = match.group(3) or ""

            # Call the corresponding method based on the placeholder name
            return placeholders[placeholder].strip() + endl

        return re.sub(r"(\s*)@(.*?)@(\n?)", replace_placeholder, TEMPLATE_CODE)

    def get_enums_text(self, indent):
        tab = INDENT * indent

        return ",\n".join(f"{tab}{state.name}" for state in self.states)

    def get_cases_text(self, indent):
        state_code = str()

        for state in self.states:
            if isinstance(state, OperationState):
                continue
            state_code += state.to_code_case(indent)
            state_code += state.to_code(indent)

        # Create the default cases
        for state in self.states:
            if isinstance(state, OperationState):
                state_code += state.to_code_case(indent)

        return state_code

    def get_callbacks_text(self, indent):
        state_code = str()

        for state in self.states:
            if isinstance(state, OperationState):
                state_code += state.to_code_case(indent+1)
                state_code += state.to_code(indent+2)

        for state in self.states:
            if not isinstance(state, OperationState):
                state_code += state.to_code_case(indent+1)

        return state_code

    def get_prototypes(self, indent):
        tab = INDENT * indent
        retval = str()

        for name, proto in self.callbacks.items():
            retval += f"{tab}callback_outcome_t {name}("

            for idx, param in enumerate(proto):
                if isinstance(param, tuple):
                    # Skip the name
                    param, param_name = param
                    param_name = " " + param_name
                else:
                    param_name = ""

                comma = ", " if len(proto) - idx > 1 else ""

                retval += f"{param(0).ctype}{param_name}{comma}"

            retval += ");\n"

        return retval

    def process_callbacks(self, callback_list):
        """ Create a lookup for all the devices including param names """
        for cb, proto in callback_list.items():
            # Check the cb name is C
            if not VALID_C_FUNCTION_NAME.match(cb):
                raise ParsingException("Callback name is not a valid C function name")

            self.callbacks[cb] = proto

    def process_devices(self, tree):
        """ Start the process with grouping all the devices """
        current_state = self.new_state("DEVICE_ADDRESS", 0)

        for key, value in tree.items():
            if key.startswith("device@"):
                match = re.search(DEVICE_ADDR_RE, key)
                if not match:
                    raise ParsingException("Malformed device address")
                device_number = int(match.group(1), 0)
                if device_number > 255:
                    raise ParsingException("device address must be < 256")

                device_state = self.new_state(f"DEVICE_{device_number}", 1)
                address_matcher = u8(device_number, alias=device_state.name)
                current_state.add(Transition(address_matcher, device_state))

                for command in value:
                    self.process_sequence(address_matcher, device_state, command)

    def process_sequence(self, address_matcher, state, cmd):
        """ Given a single sequence, create the states and transitions """
        pos = 1 # First byte
        callback = cmd[-1]
        if callback not in self.callbacks:
            raise ParsingException(f"Unknown callback {callback}: Callback must be declared first")

        for index, matcher in enumerate(cmd):
            # Size of the matcher
            pos += matcher.size
            matcher.pos = pos  # Set the position at which the matcher matches

            if state.has(matcher):
                state = state.get_next_state_of(matcher)
            else:
                if isinstance(cmd[index+1], str): # Command to follow?
                    next_state = self.new_state(state.next(matcher.alias), state.pos + matcher.size)
                    state.add(Transition(matcher, next_state))
                    state = next_state

                    command_name = cmd[-1] # Grab the command name

                    if command_name not in self.callbacks:
                        raise ParsingException(f"Cmd {command_name} does not have a prototype")

                    # Add the CRC calculation
                    next_state = self.new_state(state.next("_" + command_name.upper() + "__CRC"), state.pos + matcher.size)
                    to_crc_transition = Transition(matcher, next_state)
                    to_crc_transition.set_crc = True
                    state.add(to_crc_transition)
                    state = next_state

                    # Add the final transition before making the call to the callback
                    op = Operation(command_name, self.callbacks[command_name], [address_matcher] + list(cmd[:-1]))
                    next_state = OperationState(op, "RDY_TO_CALL__" + command_name.upper(), 0)
                    self.states.append(next_state)
                    crc_matcher = Crc(None)
                    state.add(Transition(crc_matcher, next_state))
                    break
                else:
                    next_state = self.new_state(state.next(matcher.alias), state.pos + matcher.size)
                    state.add(Transition(matcher, next_state))
                    state = next_state

if __name__ == "__main__":
    print("Call the using file")

class Modbus:
    def __init__(self, modbus):
        self.modbus = modbus
        self.main()

    def main(self):
        import argparse
        parser = argparse.ArgumentParser(description="Generate code for Modbus.")
        parser.add_argument('-o', '--output', type=str, help='Output file name')
        args = parser.parse_args()

        try:
            gen = CodeGenerator(self.modbus)
            generated_code = gen.generate_code()
        except ParsingException as e:
            print( "Error: " + str(e) )
        else:
            if args.output:
                with open(args.output, 'w') as f:
                    f.write(generated_code)
            else:
                print(generated_code)
