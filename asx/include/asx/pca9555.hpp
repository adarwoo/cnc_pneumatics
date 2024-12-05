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
   
   template<uint8_t ADDRESS, uint16_t dir, uint16_t pol>
   class PCA9555
   {
      ///< Address of the device
      static constexpr uint8_t address = base_address | ADDRESS;
      ///< Buffer to receive data
      uint8_t buffer[2];

   public:
      static init(reactor::Handle complete) {
         static auto bind_this = [this, complete]() {
            this->init(complete);
         };

         static auto react_this = reactor::bind(bind_this);

         if ( init_stage == conf_dir ) {
            I2CM::write(...., react_this, state::conf_pol, );
         } else if () {
            I2CM::write(...., reactor::bind(init), state::conf_done, );
         } else {
            reactor::notify(complete);
         }
      }

      void read(reactor::handle on_complete); // Pass a uint16_t
      void readwrite(uint16_t to_write, reactor::handle on_complete); // Pass a uint16_t

   private:
      void write(command_type, uint16_t);
   };   


   using Pca1 = PCA9555<2, 0xFFFF, 0>;
   using Pca2 = PCA9555<3, 0, 0xFFFF>;

   // The main app owns the i2c

   // Cascade the initialisation of both i2c
   auto react_on_sample_i2c = reactor::bind(on_sample_i2c);

   auto iomux_1 = i2cPca1.init(reactor::bind(i2cPca2.init(react_on_sample_i2c));

   void on_sample_i2c()
   {
      static auto react_on_pca1_result = reactor::bind(on_pca1_result);

      i2cPca1.readwrite(fb.low, react_on_pca1_result);
   }
   

   auto pca2 = i2cmaster[0]; // Get the first slave

   pca1.request(on_)

   // In main



}
