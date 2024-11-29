#!/usr/bin/env python3
from modbus_rtu_slave_rc import *  # Import everything from modbus_generator

# Coils = LED+
# Discrete inputs = Switch state
# Input register = Message queue

# 0x00 Start
# 0x01 Stop
# 0x02 Homing
# 0x03 Goto0
# 0x04 Park
# 0x05 Chuck
# 0x06 Door
# 0x07 PGM
# 0x10 OvDoor
# 0x11 OvDust
# 0x12 OvCool
# 0x13 OvFree


Modbus({
    "buffer_size": 32,
    "namespace": "relay",

    "callbacks": {
        "on_get_leds_status": [(u8, "from"), (u8, "qty")],
        "on_get_sw_status":   [(u8, "from"), (u8, "qty")],
        "on_set_single_led":  [(u8, "led_at"), (u16, "operation")],
        "on_write_leds":      [(u8, "from"), (u8, "qty")],
        "on_read_queue":      []
    },

    "device@0x1A": [
        # Get the LED status
        (READ_COILS,            u16(0, 11, alias="from"),
                                u16(1, 11, alias="qty"), # Byte count
                                "on_get_leds_status"),

        # Read the push button and switches state
        (READ_DISCRETE_INPUTS,  u16(0, 11, alias="from"),
                                u16(1, 11, alias="qty"), # Byte count
                                "on_get_sw_status"),

        # Turn an LED on or off
        (WRITE_SINGLE_COIL,     u16(0, 11, alias="index"),
                                u16([0xFF00, 0], alias="op"),
                                "on_set_single_led"),

        (WRITE_MULTIPLE_COILS,  u16(0, 11, alias="from"),
                                u16(1, 11, alias="qty"), # Byte count
                                "on_write_leds"),

        # Returns FFFF if nothing. Returns FFxx for Alt keys
        (READ_INPUT_REGISTERS,  u16(0x0000),
                                u16(1),
                                "on_read_queue"),
    ]
})
