#!/usr/bin/env python3
from modbus_rtu_slave_rc import *  # Import everything from modbus_generator

Modbus({
    "buffer_size": 32,
    "namespace": "relay",

    "callbacks": {
        "on_get_status":        [(u8, "relay_index"), (u8, "operation")],
        "on_set_single":        [(u8, "relay_index"), (u16, "operation")],
        "on_write_all":         [(u16, "operation")],
        "on_read_version":      [],
    },

    "device@44": [
        (READ_COILS,            u16(alias="address"),
                                u16(1), # Valid command is 1
                                "on_get_status"),
        (WRITE_SINGLE_COIL,     u16(0, 2, alias="ID"),
                                u16([0xFF00, 0, 0x5500], alias="OP"),
                                "on_set_single"),
        (WRITE_SINGLE_COIL,     u16(0xFF, alias="ID"),
                                u16([0xFF00, 0, 0x5500], alias="OP"),
                                "on_write_all"),
        (READ_HOLDING_REGISTERS, u16(1), "on_read_version"),
    ]
})
