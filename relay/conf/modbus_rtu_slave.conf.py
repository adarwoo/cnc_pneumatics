#!/usr/bin/env python3
from modbus_rtu_slave_rc import *  # Import everything from modbus_generator

Modbus({
    "callbacks": {
        "on_get_single":                [],
        "on_set_single":                [(u8, "relay_index"), (u8, "operation")],
        "on_write_all":                 [u8],
        "on_read_version":              [],
        "on_read_baud_rate":            [],
        "on_read_stop_and_data_bits":   [],
        "on_read_parity_bits":          [],
        "on_read_response_delay":       [],
        "on_read_modbus_mode":          [],
        "on_read_watchdog":             []
    },

    "device@44": [
        (READ_COILS,            u16(0, 3, alias="ID"),
                                "on_get_single"),
        (WRITE_SINGLE_COIL,     u16(0, 3, alias="ID"),
                                u16([0xFF, 0, 0x55], alias="OP"),
                                "on_set_single"),
        (WRITE_SINGLE_COIL,     u16(0xFF, alias="ID"),
                                u16([0xFF, 0, 0x55], alias="OP"), 
                                "on_write_all"),
        (READ_HOLDING_REGISTERS, u16(1), "on_read_version"),
        (READ_INPUT_REGISTERS,   u16(1), "on_read_baud_rate"),
        (READ_INPUT_REGISTERS,   u16(2), "on_read_stop_and_data_bits"),
        (READ_INPUT_REGISTERS,   u16(3), "on_read_parity_bits"),
        (READ_INPUT_REGISTERS,   u16(5), "on_read_response_delay"),
        (READ_INPUT_REGISTERS,   u16(7), "on_read_modbus_mode"),
        (READ_INPUT_REGISTERS,   u16(9), "on_read_watchdog"),
    ]
})
