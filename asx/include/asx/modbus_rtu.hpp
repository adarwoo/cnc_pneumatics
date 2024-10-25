#pragma once

#include <stdint.h>

namespace modbus
{
    enum class process_outcomme_t : uint8_t {
        ignore, // Wait for the stream to stop (3.5T) as it is not for us
        expecting_more, // Char was processed, another is expected
        unsupported_operation, // Operation is not supported
        invalid_value, // Value is out-of-range or not valid
        reply, // Reply is ready to send
    };
}
