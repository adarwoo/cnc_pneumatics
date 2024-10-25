/**
 * This file was generated to create a state machine for processing
 * uart data used for a modbus RTU
 */
#include <cstdint>
#include <cassert>
#include <iostream>
#include <iomanip>

namespace modbus {
    namespace slave {
        enum class process_outcomme_t : uint8_t {
            ignore, // Wait for the stream to stop (3.5T) as it is not for us
            expecting_more, // Char was processed, another is expected
            unsupported_operation, // Operation is not supported
            invalid_value, // Value is out-of-range or not valid
            reply, // Reply is ready to send
        };

        process_outcomme_t on_read_coil(uint8_t coil);
        process_outcomme_t on_write_coil(uint8_t relay, uint8_t);
        process_outcomme_t on_write_all_coils(uint8_t);
        process_outcomme_t on_read_version();
        process_outcomme_t on_read_baud_rate();
        process_outcomme_t on_read_stop_and_data_bits();
        process_outcomme_t on_read_parity_bits();
        process_outcomme_t on_read_response_delay();
        process_outcomme_t on_read_modbus_mode();
        process_outcomme_t on_read_watchdog();

        enum class state_t : uint8_t {
            DEVICE_ADDRESS,
            DEVICE_44,
            DEVICE_44_READ_COILS,
            DEVICE_44_WRITE_SINGLE_COIL,
            DEVICE_44_WRITE_SINGLE_COIL_ID,
            DEVICE_44_WRITE_SINGLE_COIL_2,
            DEVICE_44_READ_HOLDING_REGISTERS,
            DEVICE_44_READ_INPUT_REGISTERS
        };

        struct Processor {
            state_t state;
            uint8_t idx;

            uint8_t buffer[9];

            void reset() {
                state = state_t::DEVICE_ADDRESS;
                idx = 0;
            }

            Processor() {
                reset();
            }

            void dump_buffer() const {
                std::cout << "Buffer: ";
                for (uint8_t i = 0; i < idx; ++i) {
                    std::cout << "0x"
                              << std::setw(2) << std::setfill('0') << std::hex
                              << static_cast<int>(buffer[i]) << " ";
                }
                std::cout << std::dec << std::endl; // Switch back to decimal output
            }

            auto process(const uint8_t c) -> process_outcomme_t {
                buffer[idx++] = c; // Store the data

                switch(state) {
                case state_t::DEVICE_ADDRESS:
                    if ( c == 44 ) {
                        state = state_t::DEVICE_44;
                    } else {
                        return process_outcomme_t::ignore;
                    }
                    break;
                case state_t::DEVICE_44:
                    if ( c == 1 ) {
                        state = state_t::DEVICE_44_READ_COILS;
                    } else if ( c == 5 ) {
                        state = state_t::DEVICE_44_WRITE_SINGLE_COIL;
                    } else if ( c == 3 ) {
                        state = state_t::DEVICE_44_READ_HOLDING_REGISTERS;
                    } else if ( c == 4 ) {
                        state = state_t::DEVICE_44_READ_INPUT_REGISTERS;
                    } else {
                        return process_outcomme_t::unsupported_operation;
                    }
                    break;
                case state_t::DEVICE_44_READ_COILS:
                    if ( idx == 4 ) {
                        uint8_t *data = &buffer[idx-2];
                        uint16_t c = (data[0] << 8) | data[1];
                        printf("Data: %d - %d IDX=%d\n", *data, c, idx);
                        dump_buffer();
                        if ( c < 3 ) {
                            return on_read_coil(c);
                        } else {
                            return process_outcomme_t::invalid_value;
                        };
                    }
                    break;
                case state_t::DEVICE_44_WRITE_SINGLE_COIL:
                    if ( idx == 4 ) {
                        uint8_t *data = &buffer[idx-2];
                        uint16_t c = (data[0] << 8) | data[1];
                        if ( c < 3 ) {
                            state = state_t::DEVICE_44_WRITE_SINGLE_COIL_ID;
                        } else if ( c == 255 ) {
                            state = state_t::DEVICE_44_WRITE_SINGLE_COIL_2;
                        } else {
                            return process_outcomme_t::invalid_value;
                        };
                    }
                    break;
                case state_t::DEVICE_44_WRITE_SINGLE_COIL_ID:
                    if ( idx == 6 ) {
                        uint8_t *data = &buffer[idx-2];
                        uint16_t c = (data[0] << 8) | data[1];
                        if ( c == 255 || c == 0 || c == 85 ) {
                            return on_write_coil(uint8_t{buffer[3]}, uint8_t{buffer[5]});
                        } else {
                            return process_outcomme_t::invalid_value;
                        };
                    }
                    break;
                case state_t::DEVICE_44_WRITE_SINGLE_COIL_2:
                    if ( idx == 6 ) {
                        uint8_t *data = &buffer[idx-2];
                        uint16_t c = (data[0] << 8) | data[1];
                        if ( c == 255 || c == 0 || c == 85 ) {
                            return on_write_all_coils(uint8_t{buffer[5]});
                        } else {
                            return process_outcomme_t::invalid_value;
                        };
                    }
                    break;
                case state_t::DEVICE_44_READ_HOLDING_REGISTERS:
                    if ( idx == 4 ) {
                        uint8_t *data = &buffer[idx-2];
                        uint16_t c = (data[0] << 8) | data[1];
                        if ( c == 1 ) {
                            return on_read_version();
                        } else {
                            return process_outcomme_t::invalid_value;
                        };
                    }
                    break;
                case state_t::DEVICE_44_READ_INPUT_REGISTERS:
                    if ( idx == 4 ) {
                        uint8_t *data = &buffer[idx-2];
                        uint16_t c = (data[0] << 8) | data[1];
                        if ( c == 1 ) {
                            return on_read_baud_rate();
                        } else if ( c == 2 ) {
                            return on_read_stop_and_data_bits();
                        } else if ( c == 3 ) {
                            return on_read_parity_bits();
                        } else if ( c == 5 ) {
                            return on_read_response_delay();
                        } else if ( c == 7 ) {
                            return on_read_modbus_mode();
                        } else if ( c == 9 ) {
                            return on_read_watchdog();
                        } else {
                            return process_outcomme_t::invalid_value;
                        };
                    }
                    break;

                default:
                    break;
                }

                return process_outcomme_t::expecting_more;
            }
        }; // struct Processor

        // Mock implementations for callbacks
        process_outcomme_t on_read_coil(uint8_t c) {
            std::cout << "on_read_coil called" << std::endl;
            return process_outcomme_t::reply;
        }

        process_outcomme_t on_write_coil(uint8_t relay, uint8_t val) {
            std::cout << "on_write_coil called with relay: " << int(relay) << ", value: " << int(val) << std::endl;
            return process_outcomme_t::reply;
        }

        process_outcomme_t on_write_all_coils(uint8_t val) {
            std::cout << "on_write_all_coils called with value: " << int(val) << std::endl;
            return process_outcomme_t::reply;
        }

        process_outcomme_t on_read_version() {
            std::cout << "on_read_version called" << std::endl;
            return process_outcomme_t::reply;
        }

        process_outcomme_t on_read_baud_rate() {
            std::cout << "on_read_baud_rate called" << std::endl;
            return process_outcomme_t::reply;
        }

        process_outcomme_t on_read_stop_and_data_bits() {
            std::cout << "on_read_stop_and_data_bits called" << std::endl;
            return process_outcomme_t::reply;
        }

        process_outcomme_t on_read_parity_bits() {
            std::cout << "on_read_parity_bits called" << std::endl;
            return process_outcomme_t::reply;
        }

        process_outcomme_t on_read_response_delay() {
            std::cout << "on_read_response_delay called" << std::endl;
            return process_outcomme_t::reply;
        }

        process_outcomme_t on_read_modbus_mode() {
            std::cout << "on_read_modbus_mode called" << std::endl;
            return process_outcomme_t::reply;
        }

        process_outcomme_t on_read_watchdog() {
            std::cout << "on_read_watchdog called" << std::endl;
            return process_outcomme_t::reply;
        }

        // Unit Tests
        void test_device_address() {
            Processor processor;

            // Test correct address (44)
            assert(processor.process(44) == process_outcomme_t::expecting_more);
            assert(processor.state == state_t::DEVICE_44);

            // Test incorrect address
            processor.reset();
            assert(processor.process(22) == process_outcomme_t::ignore);
            assert(processor.state == state_t::DEVICE_ADDRESS);
        }

        void test_read_coil() {
            Processor processor;

            // Simulate address match
            processor.process(44);
            // Simulate read coils command
            assert(processor.process(1) == process_outcomme_t::expecting_more);
            assert(processor.state == state_t::DEVICE_44_READ_COILS);

            // Provide correct coil number and check if callback is called
            assert(processor.process(0) == process_outcomme_t::expecting_more);
            assert(processor.process(0) == process_outcomme_t::reply);
        }

        void test_write_single_coil() {
            Processor processor;

            // Simulate address match
            processor.process(44);
            // Simulate write single coil command
            assert(processor.process(5) == process_outcomme_t::expecting_more);
            assert(processor.state == state_t::DEVICE_44_WRITE_SINGLE_COIL);

            // Provide valid coil ID
            assert(processor.process(0) == process_outcomme_t::expecting_more);
            assert(processor.process(1) == process_outcomme_t::expecting_more);
            assert(processor.process(0) == process_outcomme_t::expecting_more);
            assert(processor.process(85) == process_outcomme_t::reply);  // Check if callback is called with correct values
        }

        void test_read_holding_registers() {
            Processor processor;

            // Simulate address match
            processor.process(44);
            // Simulate read holding registers command
            assert(processor.process(3) == process_outcomme_t::expecting_more);
            assert(processor.state == state_t::DEVICE_44_READ_HOLDING_REGISTERS);

            // Provide correct register value and check callback
            assert(processor.process(0) == process_outcomme_t::expecting_more);
            assert(processor.process(1) == process_outcomme_t::reply);
        }

        void test_invalid_coil_value() {
            Processor processor;

            // Simulate address match
            processor.process(44);
            // Simulate write single coil command
            processor.process(5);

            // Provide invalid coil ID
            assert(processor.process(0) == process_outcomme_t::expecting_more);
            assert(processor.process(0) == process_outcomme_t::expecting_more);
            assert(processor.process(0) == process_outcomme_t::expecting_more);
            assert(processor.process(10) == process_outcomme_t::invalid_value);  // Invalid value
        }

        void test_all() {
            test_device_address();
            test_read_coil();
            test_write_single_coil();
            test_read_holding_registers();
            test_invalid_coil_value();
        }

    } // namespace slave
} // namespace modbus

int main() {
    modbus::slave::test_all();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
