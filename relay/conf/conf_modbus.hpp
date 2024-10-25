#pragma once

#include "conf/uart.hpp"
#include "asx/modbus.hpp"

auto on_write_coil = [](modbus_handler &handler, uint16_t value) {
   if (value < 2) {
      relay[handler.address] = value;
      return handler.echo_reply();
   }

   return modbus::error::application;
}

auto on_write_single_coil = [](modbus_handler &handler, uint16_t select, uint16_t value) {
   switch (value) {
      case 0: relays.set(select); break;
      case 255: relays.clear(select); break;
      case 0x55: relay.toggle(select) = true; break;
      default:
         break;
   }

   return handler.reply_echo();
}

auto modbus_slave = modbus<board::rs485>::create_modbus_slave<0x44>(
   modbus::read_coils             | match(0, 3)                            / on_read_coils,
   modbus::write_single_coil      | match(0, 3) + match({0xff, 0x0, 0x55}) / on_write_coil,
   modbus::write_single_coil      | match(255)  + match({0xff, 0x0, 0x55}) / on_write_all_coils,
   modbus::read_holding_registers | match(1)                               / on_read_version,
   modbus::read_input_registers   | match(2)                               / on_read_baud_rate,
   modbus::read_input_registers   | match(3)                               / on_read_stop_and_data_bits,
   modbus::read_input_registers   | match(5)                               / on_read_parity_bits,
   modbus::read_input_registers   | match(6)                               / on_read_response_delay,
   modbus::read_input_registers   | match(7)                               / on_read_modbus_mode,
   modbus::read_input_registers   | match(9)                               / on_read_watchdog,
);

static int8_t position = 0;

modbus::status_t process_char(uint8_t c)
{
   static int8_t idx = -1; // Max 254 chars
   static uint8_t buf[MAX_INDEX ] = 0; // Max index can be determined during the construction of the function

   idx++; // Move the index
   
   // Grab the current char
   uint8_t c = in[index];
   // Lambda gets the last received value in hton format
   // Template lambda to convert network byte order to host byte order
   template <typename T>
   auto ntoh =  -> T {
      static_assert(std::is_integral_v<T>, "T must be an integral type");
      T value = 0;

      for (size_t i = 0; i < sizeof(T); ++i) {
         value |= static_cast<T>(buf[idx - i]) << (i * 8);
      }

      return value;
   };
    
   switch ( state ) {
   case DEVICE_3:
     if c == modbus::read_coils:
        return DEVICE_3_READ_COILS;
     if c == modbus::write_single_coil:
        return 2;
     if c == modbus::read_holding_registers:
        return 3;
     if c == modbus::read_input_registers:
        return 4;
     return -1;
   case DEVICE_3_READ_COILS: // case 1 : read coils
     if idx < 2:
        return 1; // Get me another char
     if c.match_range<uint16_t, 0, 3>(ntoh<uint16_t>()) {
         auto pack = ntoh<uint8_t, uint16_t>();
         return std::apply(on_read_coils, pack);
     }

     return -1;
   case 2: // write single coil
     if idx < 2:
        return 2; // Get me another char
     if c.match_range<uint16_t, 0, 3>(ntoh<uint16_t>()):
        return 6;
     if c == match<uint16_t, 255>(ntoh<uint16_t>()):
        return 7;
     return -1;
   case 3: // Read holding
     if idx < 2:
        return 3; // Get me another char
     if c == match<uint16_t, 1)(ntoh<uint16_t>()):
        return 8;
     return -1;
   case 4: // Read input
     if idx < 2:
        return 4; // Get me another char

     if c == match<uint16_t, 1>(buf-1):
        return check_crc_and_call(on_read_version);
     if c == match<uint16_t, 2>(buf-1):
        return 10;
     if c == match<uint16_t, 3>(buf-1):
        return 11;
     if c == match<uint16_t, 5>(buf-1):
        return 12;
     if c == match<uint16_t, 6>(buf-1):
        return 13;
     if c == match<uint16_t, 7>(buf-1):
        return 14;
     if c == match<uint16_t, 9>(buf-1):
        return 15;

     return -1;
   case 5: // Read Coil match range 

auto modbus_slave = modbus<board::rs485>::create_modbus_slave<0x44>(
   modbus::read_coils(
      match(0, 3) / on_read_coils,
   ),
   modbus::write_single_coil(
      match_range<0, 3>(
         match<0>() / turn_relay_off,
         match<255>() / turn_relay_on,
         match<0xAA>() / toggle_relay
      ),
      match<255>(
         match<0>() / turn_all_relays_off,
         match<255>() / turn_all_relays_on,
         match<0xAA>() / toggle_all_relays
      )
   ),
   modbus::read_holding_registers(
      match<1>() / on_read_version
   ),
   modbus::read_input_registers(
      match<2>()            / on_read_baud_rate,
      match<3>()            / on_read_stop_and_data_bits,
      match<5>()            / on_read_parity_bits,
      match<6>()            / on_read_response_delay,
      match<7>()            / on_read_modbus_mode,
      match<9>()            / on_read_watchdog,
      match<uint32_t{33}>() / on_read_received_packets
      match<uint32_t{34}>() / on_read_errored_packets
   )
);

