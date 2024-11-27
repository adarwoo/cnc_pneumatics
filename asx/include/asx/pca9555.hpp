#include <stdint.h>

namespace pca9555
{
   static constexpr auto base_address = 0b0100000;

   enum command_type : uint8_t
   {
      read = 0,
      write = 2,
      set_polarity = 4,
      configure = 6,
   };
   
   class pca9555
   {
      ///< Address of the device
      uint8_t address;
      ///< Buffer to receive data
      uint8_t buffer[2];
      ///< TWI device to use

   public:
      pca9555(uint8_t _address) : address(_address) {}
      void write(command_type, uint16_t value);
   };   
}
