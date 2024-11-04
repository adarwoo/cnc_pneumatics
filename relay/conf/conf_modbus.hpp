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
