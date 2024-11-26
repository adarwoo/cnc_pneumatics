/**
 * This file was generated to create a state machine for processing
 * uart data used for a modbus RTU. It should be included by
 * the modbus_rtu_slave.cpp file only which will create a full rtu slave device.
 */
#include <logger.h>
#include <stdint.h>
#include <asx/modbus_rtu.hpp>

namespace relay {
    // All callbacks registered
    void on_get_status(uint8_t relay_index, uint8_t operation);
    void on_set_single(uint8_t relay_index, uint8_t operation);
    void on_write_all(uint8_t operation);
    void on_read_version();

    // All states to consider
    enum class state_t : uint8_t {
        IGNORE = 0,
        ERROR = 1,
        DEVICE_ADDRESS,
        DEVICE_44,
        DEVICE_44_READ_COILS,
        DEVICE_44_READ_COILS_address,
        DEVICE_44_READ_COILS_address__ON_GET_STATUS__CRC,
        RDY_TO_CALL__ON_GET_STATUS,
        DEVICE_44_WRITE_SINGLE_COIL,
        DEVICE_44_WRITE_SINGLE_COIL_ID,
        DEVICE_44_WRITE_SINGLE_COIL_ID__ON_SET_SINGLE__CRC,
        RDY_TO_CALL__ON_SET_SINGLE,
        DEVICE_44_WRITE_SINGLE_COIL_ID_1,
        DEVICE_44_WRITE_SINGLE_COIL_ID_1__ON_WRITE_ALL__CRC,
        RDY_TO_CALL__ON_WRITE_ALL,
        DEVICE_44_READ_HOLDING_REGISTERS,
        DEVICE_44_READ_HOLDING_REGISTERS__ON_READ_VERSION__CRC,
        RDY_TO_CALL__ON_READ_VERSION
    };

    class Datagram {
        using error_t = asx::modbus::error_t;

        ///< Adjusted buffer to only receive the largest amount of data possible
        inline static uint8_t buffer[32];
        ///< Number of characters in the buffer
        inline static uint8_t cnt;
        ///< Number of characters to send
        inline static uint8_t frame_size;
        ///< Error code
        inline static error_t error;
        ///< State
        inline static state_t state;
        ///< CRC for the datagram
        inline static asx::modbus::Crc crc{};


    public:
        // Status of the datagram
        enum class status_t : uint8_t {
            GOOD_FRAME = 0,
            NOT_FOR_ME = 1,
            BAD_CRC = 2
        };

        static void reset() noexcept {
            cnt=0;
            crc.reset();
            error = error_t::ok;
            state = state_t::DEVICE_ADDRESS;
        }

        static status_t get_status() noexcept {
            if (state == state_t::IGNORE) {
                return status_t::NOT_FOR_ME;
            }

            return crc.check() ? status_t::GOOD_FRAME : status_t::BAD_CRC;
        }

        static void process_char(const uint8_t c) noexcept {
            LOG_TRACE("DGRAM", "Char: 0x%.2x, index: %d, state: %d", c, cnt, (uint8_t)state);

            if (state == state_t::IGNORE) {
                return;
            }

            crc(c);

            if (state != state_t::ERROR) {
                // Store the frame
                buffer[cnt++] = c; // Store the data
            }

            switch(state) {
            case state_t::ERROR:
                break;
            case state_t::DEVICE_ADDRESS:
                if ( c == 44 ) {
                    state = state_t::DEVICE_44;
                } else {
                    error = error_t::ignore_frame;
                    state = state_t::IGNORE;
                }
                break;
            case state_t::DEVICE_44:
                if ( c == 1 ) {
                    state = state_t::DEVICE_44_READ_COILS;
                } else if ( c == 5 ) {
                    state = state_t::DEVICE_44_WRITE_SINGLE_COIL;
                } else if ( c == 3 ) {
                    state = state_t::DEVICE_44_READ_HOLDING_REGISTERS;
                } else {
                    error = error_t::illegal_function_code;
                    state = state_t::ERROR;
                }
                break;
            case state_t::DEVICE_44_READ_COILS:
                if ( cnt == 4 ) {
                    state = state_t::DEVICE_44_READ_COILS_address;;
                }
                break;
            case state_t::DEVICE_44_READ_COILS_address:
                if ( cnt == 6 ) {
                    uint8_t *data = &buffer[cnt-2];
                    uint16_t c = (data[0] << 8) | data[1];

                    if ( c == 1 ) {
                        state = state_t::DEVICE_44_READ_COILS_address__ON_GET_STATUS__CRC;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_READ_COILS_address__ON_GET_STATUS__CRC:
                if ( cnt == 8 ) {
                    state = state_t::RDY_TO_CALL__ON_GET_STATUS;
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_COIL:
                if ( cnt == 4 ) {
                    uint8_t *data = &buffer[cnt-2];
                    uint16_t c = (data[0] << 8) | data[1];

                    if ( c < 3 ) {
                        state = state_t::DEVICE_44_WRITE_SINGLE_COIL_ID;
                    } else if ( c == 255 ) {
                        state = state_t::DEVICE_44_WRITE_SINGLE_COIL_ID_1;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_COIL_ID:
                if ( cnt == 6 ) {
                    uint8_t *data = &buffer[cnt-2];
                    uint16_t c = (data[0] << 8) | data[1];

                    if ( c == 255 || c == 0 || c == 85 ) {
                        state = state_t::DEVICE_44_WRITE_SINGLE_COIL_ID__ON_SET_SINGLE__CRC;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_COIL_ID__ON_SET_SINGLE__CRC:
                if ( cnt == 8 ) {
                    state = state_t::RDY_TO_CALL__ON_SET_SINGLE;
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_COIL_ID_1:
                if ( cnt == 6 ) {
                    uint8_t *data = &buffer[cnt-2];
                    uint16_t c = (data[0] << 8) | data[1];

                    if ( c == 255 || c == 0 || c == 85 ) {
                        state = state_t::DEVICE_44_WRITE_SINGLE_COIL_ID_1__ON_WRITE_ALL__CRC;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_COIL_ID_1__ON_WRITE_ALL__CRC:
                if ( cnt == 8 ) {
                    state = state_t::RDY_TO_CALL__ON_WRITE_ALL;
                }
                break;
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS:
                if ( cnt == 4 ) {
                    uint8_t *data = &buffer[cnt-2];
                    uint16_t c = (data[0] << 8) | data[1];

                    if ( c == 1 ) {
                        state = state_t::DEVICE_44_READ_HOLDING_REGISTERS__ON_READ_VERSION__CRC;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS__ON_READ_VERSION__CRC:
                if ( cnt == 6 ) {
                    state = state_t::RDY_TO_CALL__ON_READ_VERSION;
                }
                break;
            case state_t::RDY_TO_CALL__ON_GET_STATUS:
            case state_t::RDY_TO_CALL__ON_SET_SINGLE:
            case state_t::RDY_TO_CALL__ON_WRITE_ALL:
            case state_t::RDY_TO_CALL__ON_READ_VERSION:
            default:
                error = error_t::illegal_data_value;
                state = state_t::ERROR;
                break;
            }
        }

        static void reply_error( error_t err ) noexcept {
            buffer[1] |= 0x80;
            buffer[3] = (uint8_t)err;
        }

        template<typename T>
        static void pack(const T& value) noexcept {
            if constexpr ( sizeof(T) == 1 ) {
                buffer[cnt++] = value;
            } else if constexpr ( sizeof(T) == 2 ) {
                buffer[cnt++] = value >> 8;
                buffer[cnt++] = value & 0xff;
            } else if constexpr ( sizeof(T) == 4 ) {
                buffer[cnt++] = value >> 24;
                buffer[cnt++] = value >> 16 & 0xff;
                buffer[cnt++] = value >> 8 & 0xff;
                buffer[cnt++] = value & 0xff;
            }
        }

        /** Called when a T3.5 has been detected, in a good sequence */
        static void ready_reply() noexcept {
            frame_size = cnt; // Store the frame size
            cnt = 2; // Points to the function code

            switch(state) {
            case state_t::IGNORE:
                break;
            case state_t::DEVICE_ADDRESS:
            case state_t::DEVICE_44:
            case state_t::DEVICE_44_READ_COILS:
            case state_t::DEVICE_44_READ_COILS_address:
            case state_t::DEVICE_44_READ_COILS_address__ON_GET_STATUS__CRC:
            case state_t::DEVICE_44_WRITE_SINGLE_COIL:
            case state_t::DEVICE_44_WRITE_SINGLE_COIL_ID:
            case state_t::DEVICE_44_WRITE_SINGLE_COIL_ID__ON_SET_SINGLE__CRC:
            case state_t::DEVICE_44_WRITE_SINGLE_COIL_ID_1:
            case state_t::DEVICE_44_WRITE_SINGLE_COIL_ID_1__ON_WRITE_ALL__CRC:
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS:
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS__ON_READ_VERSION__CRC:
                error = error_t::illegal_data_value;
            case state_t::ERROR:
                buffer[cnt++] |= 0x80; // Mark the error
                buffer[cnt++] = (uint8_t)error; // Add the error code
                break;
            case state_t::RDY_TO_CALL__ON_GET_STATUS:
                on_get_status(uint8_t{buffer[3]}, uint8_t{buffer[5]});
                break;
            case state_t::RDY_TO_CALL__ON_SET_SINGLE:
                on_set_single(uint8_t{buffer[3]}, uint8_t{buffer[5]});
                break;
            case state_t::RDY_TO_CALL__ON_WRITE_ALL:
                on_write_all(uint8_t{buffer[5]});
                break;
            case state_t::RDY_TO_CALL__ON_READ_VERSION:
                on_read_version();
                break;
            default:
                break;
            }

            // If the cnt is 2 - nothing was changed in the buffer - return it as is
            if ( cnt == 2 ) {
                cnt = frame_size; // Framesize includes the previous CRC which still holds valid
            } else {
                // Add the CRC
                crc.reset();
                auto _crc = crc.update(std::string_view{(char *)buffer, cnt});
                buffer[cnt++] = _crc & 0xff;
                buffer[cnt++] = _crc >> 8;
            }
        }

        static std::string_view get_buffer() noexcept {
            // Return the buffer ready to send
            return std::string_view{(char *)buffer, cnt};
        }
    }; // struct Processor
} // namespace modbus