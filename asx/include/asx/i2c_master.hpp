#pragma once

#include <cmath>

#include <alert.h>
#include <asx/reactor.hpp>
#include <asx/i2c_common.hpp>

#define TWI_BAUD(freq, t_rise) ((F_CPU / freq) / 2) - (5 + (((F_CPU / 1000000) * t_rise) / 2000))

namespace asx {
    namespace i2c {

        /// Helper which computes the value of the BAUD register based on the frequency.
        constexpr uint8_t calc_baud(uint32_t frequency)
        {
            int16_t baud;

        #if (F_CPU == 20000000) || (F_CPU == 10000000)
            if (frequency >= 600000)
            { // assuming 1.5kOhm
                baud = TWI_BAUD(frequency, 250);
            }
            else if (frequency >= 400000)
            { // assuming 2.2kOhm
                baud = TWI_BAUD(frequency, 350);
            }
            else
            {									 // assuming 4.7kOhm
                baud = TWI_BAUD(frequency, 600); // 300kHz will be off at 10MHz. Trade-off between size and accuracy
            }
        #else
            if (frequency >= 600000)
            { // assuming 1.5kOhm
                baud = TWI_BAUD(frequency, 250);
            }
            else if (frequency >= 400000)
            { // assuming 2.2kOhm
                baud = TWI_BAUD(frequency, 400);
            }
            else
            { // assuming 4.7kOhm
                baud = TWI_BAUD(frequency, 600);
            }
        #endif

        #if (F_CPU >= 20000000)
            const uint8_t baudlimit = 2;
        #elif (F_CPU == 16000000) || (F_CPU == 8000000) || (F_CPU == 4000000)
            const uint8_t baudlimit = 1;
        #else
            const uint8_t baudlimit = 0;
        #endif

            if (baud < baudlimit)
            {
                return baudlimit;
            }
            else if (baud > 255)
            {
                return 255;
            }

            return (uint8_t)baud;
        }

        // Define the user-defined literal for kilohertz
        constexpr unsigned long long operator""_KHz(unsigned long long v) {
            return static_cast<unsigned long long>(v * 1000L);
        }

        // Define the user-defined literal for megahertz
        constexpr unsigned long long operator""_MHz(unsigned long long v) {
            return static_cast<unsigned long long>(v * 1000000L);
        }

        // Define the user-defined literal for kilohertz
        constexpr unsigned long long operator""_KHz(long double v) {
            return static_cast<unsigned long long>(std::round(v * 1000L));
        }

        // Define the user-defined literal for megahertz
        constexpr unsigned long long operator""_MHz(long double v) {
            return static_cast<unsigned long long>(std::round(v * 1000000L));
        }

        class Master {
            /// TWI device to use
            static inline Package *pkg = nullptr;

            /// @brief TWI Instance
            static inline int8_t addr_count;	   // Bus transfer address data counter
            static inline uint8_t data_count;	   // Bus transfer payload data counter
            static inline bool read;			   // Bus transfer direction
            static inline volatile status_code_t status; // Transfer status

        public:
            static void init(const unsigned long bus_speed_hz) {
                TWI0.MBAUD = calc_baud(bus_speed_hz);
                TWI0.MCTRLB |= TWI_FLUSH_bm;
                TWI0.MCTRLA = TWI_RIEN_bm | TWI_WIEN_bm | TWI_ENABLE_bm;
                TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;

                status = status_code_t::STATUS_OK;                
            }

            /// \brief Enable Master Mode of the TWI.
            static inline void enable() {
                TWI0.MCTRLA |= TWI_ENABLE_bm;
            }

            /// \brief Disable Master Mode of the TWI.
            static inline void disable(TWI_t *twi) {
                TWI0.MCTRLA &= (~TWI_ENABLE_bm);
            }

            /// @brief Initiate a i2c transfer (read/write/read+write)
            /// The function initiate the transfer and return immediatly.
            /// The reactor handle in the package is notified when the transfer is complete
            /// @param package The package holding the information about the transfer
            /// @param _read If true, reads, else write or write+read
            static void transfer(Package &package, bool _read = false) {
                pkg = &package;
                addr_count = 0;
                data_count = 0;
                read = _read;

                uint8_t const chip = (pkg->chip) << 1;

                // This API uses a reactor - so no colision should ever occur
                alert_and_stop_if( not is_idle() );

                // Writting the MADDR register kick starts things
                if (pkg->addr_length || (false == read)) {
                    TWI0.MADDR = chip;
                } else if (read) {
                    TWI0.MADDR = chip | 0x01;
                }
            }
            
            /// \brief Test for an idle bus state.
            static inline bool is_idle () {
                return ((TWI0.MSTATUS & TWI_BUSSTATE_gm)
                        == TWI_BUSSTATE_IDLE_gc);
            }
        private:
            /**
             * \internal
             *
             * \brief TWI master write interrupt handler.
             *
             *  Handles TWI transactions (master write) and responses to (N)ACK.
             */
            static inline void write_handler()
            {
                if (addr_count < pkg->addr_length)
                {
                    const uint8_t *const data = pkg->addr;
                    TWI0.MDATA = data[addr_count++];
                }
                else if (data_count < pkg->length)
                {

                    if (read)
                    {
                        // Send repeated START condition (Address|R/W=1)
                        TWI0.MADDR |= 0x01;
                    }
                    else
                    {
                        const uint8_t *const data = pkg->buffer;
                        TWI0.MDATA = data[data_count++];
                    }
                }
                else
                {
                    // Send STOP condition to complete the transaction
                    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
                    status = status_code_t::STATUS_OK;

                    // Notify the reactor
                    pkg->react_on_complete(status_code_t::STATUS_OK);
                }
            }            

            /**
             * \internal
             *
             * \brief TWI master read interrupt handler.
             *
             *  This is the master read interrupt handler that takes care of
             *  reading bytes from the TWI slave.
             */
            static inline void read_handler(void)
            {
                if (data_count < pkg->length) {
                    uint8_t *const data = pkg->buffer;

                    data[data_count++] = TWI0.MDATA;

                    /* If there is more to read, issue ACK and start a byte read.
                    * Otherwise, issue NACK and STOP to complete the transaction.
                    */
                    if (data_count < pkg->length) {
                        TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;
                    } else {
                        TWI0.MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;
                        status = status_code_t::STATUS_OK;

                        pkg->react_on_complete(status_code_t::STATUS_OK);
                    }
                } else {
                    /* Issue STOP and buffer overflow condition. */
                    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
                    status = status_code_t::ERR_NO_MEMORY;

                    pkg->react_on_complete(status_code_t::STATUS_OK);
                }
            }

        public: // Evil but required for the interrupt with a C linkage
            /**
             * \internal
             *
             * \brief Common TWI master interrupt service routine.
             *
             *  Check current status and calls the appropriate handler.
             */
            static inline void interrupt_handler() {
                uint8_t const master_status = TWI0.MSTATUS;

                if (master_status & TWI_ARBLOST_bm) {
                    TWI0.MSTATUS = master_status | TWI_ARBLOST_bm;
                    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
                    status = status_code_t::ERR_BUSY;

                    pkg->react_on_complete(status_code_t::ERR_BUSY);
                } else if ((master_status & TWI_BUSERR_bm) || (master_status & TWI_RXACK_bm)) {
                    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
                    status = status_code_t::ERR_IO_ERROR;

                    pkg->react_on_complete(status_code_t::ERR_IO_ERROR);
                } else if (master_status & TWI_WIF_bm) {
                    write_handler();
                } else if (master_status & TWI_RIF_bm) {
                    read_handler();
                } else {
                    status = status_code_t::ERR_PROTOCOL;

                    pkg->react_on_complete(status_code_t::ERR_PROTOCOL);
                }
            }
        };
    } // i2c
} // asx
