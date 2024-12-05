#include <asx/i2c_master.hpp>

extern "C"
{
    // Interrupt handler
    ISR(TWI0_TWIM_vect) {
        asx::i2c::Master::interrupt_handler();
    }
}
