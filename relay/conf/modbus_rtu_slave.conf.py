#!/usr/bin/env python3
from modbus_rtu_slave_rc import *  # Import everything from modbus_generator

Modbus({
    "callbacks": {
        "on_read_coil": [],
        "on_write_coil": [(u8, "relay"), u8],
        "on_write_all_coils": [u8],
        "on_read_version": [],
        "on_read_baud_rate": [],
        "on_read_stop_and_data_bits": [],
        "on_read_parity_bits": [],
        "on_read_response_delay": [],
        "on_read_modbus_mode": [],
        "on_read_watchdog": []
    },

    "device@44": [
        (READ_COILS,             u16(0, 3, alias="ID"), "on_read_coil"),
        (WRITE_SINGLE_COIL,      u16(0, 3, alias="ID"), u16([0xFF, 0, 0x55], alias="op"), "on_write_coil"),
        (WRITE_SINGLE_COIL,      u16(0xFF), u16([0xFF, 0, 0x55]), "on_write_all_coils"),
        (READ_HOLDING_REGISTERS, u16(1),    "on_read_version"),
        (READ_INPUT_REGISTERS,   u16(1),    "on_read_baud_rate"),
        (READ_INPUT_REGISTERS,   u16(2),    "on_read_stop_and_data_bits"),
        (READ_INPUT_REGISTERS,   u16(3),    "on_read_parity_bits"),
        (READ_INPUT_REGISTERS,   u16(5),    "on_read_response_delay"),
        (READ_INPUT_REGISTERS,   u16(7),    "on_read_modbus_mode"),
        (READ_INPUT_REGISTERS,   u16(9),    "on_read_watchdog"),
    ]
})
