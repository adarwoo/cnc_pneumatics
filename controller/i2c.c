/*
 * i2c.c
 *
 * Created: 14/05/2024 22:17:01
 *  Author: micro
 */
#include <avr/io.h>

#include "i2c.h"

// Reactor handle
reactor_handle_t i2c_reactor_handle;
reactor_handle_t on_error, on_data_received;

// Buffer to receive data
static uint8_t buffer; // Single byte to receive

// What was transmitted last
static opcodes_cmd_t last_sent;


/**
 * Reactor handle for when some data should be ready from the i2c
 */
static inline void _i2c_on_complete(status_code_t status)
{
   twim_release();
   
   if ( status == STATUS_OK )
   {
      // Check no transmit error
      opcodes_reply_t reply = opcodes_decode_reply(last_sent, buffer);

      if ( reply != opcodes_reply_error )
      {
         bool value = opcodes_reply_on ? true : false;

         reactor_notify(on_data_received, (void*)value);
      }
      else
      {
         status = ERR_BAD_DATA;
         reactor_notify(on_error, (void *)status);
      }
   }
   else
   {
      reactor_notify(on_error, (void *)status);
   }
}

void i2c_init(reactor_handle_t data_received, reactor_handle_t error_detected)
{
   // Store the handles
   on_error = error_detected;
   on_data_received = data_received;

   // Initialize the ASF TWI
   twi_master_init(&TWI0);
	twi_master_enable(&TWI0);
}

void i2c_master_send(opcodes_cmd_t code)
{
   static twi_package_t package;

   last_sent = code;

   package.chip = TWI_SLAVE_ADDR;
   package.addr[0] = code;
   package.addr_length = 1;
   package.buffer = &buffer;
   package.length = 1;
   package.no_wait = true; // Let the reactor take care
   package.complete_cb = _i2c_on_complete;

   // Send the read request as a repeated start to the receiver
   status_code_t status = twi_master_read(&TWI0, &package);
   
   // The reactor will have some data to process once the send is over
   // If an error is return, report it
   if ( status != STATUS_OK )
   {
      reactor_notify(on_error, (void *)status);
   }
}
