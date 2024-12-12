#include <avr/io.h>
#include "twim.h"
#include "reactor.h"
#include "pca9555.h"

namespace pca9555 {

   /**
    * Called from the interrupt
    */
   static inline void _i2c_on_complete(status_code_t status)
   {
      twim_release();
   
      if ( status == STATUS_OK )
      {
      }
      else
      {
      }
   }

   void pca9555::write(command_type cmd, uint16_t value)
   {
      static twi_package_t package;

      buffer[0] = value >> 8;
      buffer[1] = value & 0xff;

      package.chip = address | base_address;
      package.addr[0] = cmd;
      package.addr_length = 1;
      package.buffer = &buffer;
      package.length = 2;
      package.no_wait = true; // Let the reactor take care
      package.complete_cb = _i2c_on_complete;

      // Send the read request as a repeated start to the receiver
      status_code_t status = twi_master_write(&TWI0, &package);
   
      // The reactor will have some data to process once the send is over
      // If an error is return, report it
      if ( status != STATUS_OK )
      {
         //reactor_notify(on_error, (void *)status);
      }
   }
  
}