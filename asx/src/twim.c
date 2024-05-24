/**
 * \file
 *
 * \brief XMEGA TWI master source file.
 *
 * Copyright (c) 2010-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#include "sysclk.h"
#include "twim.h"


/** Master Transfer Descriptor */
static struct
{
	TWI_t *         bus;            // Bus register interface
	twi_package_t * pkg;            // Bus message descriptor
	int             addr_count;     // Bus transfer address data counter
	unsigned int    data_count;     // Bus transfer payload data counter
	bool            read;           // Bus transfer direction
	bool            locked;         // Bus busy or unavailable
	volatile status_code_t status;  // Transfer status
} transfer;


/**
 * \internal
 *
 * \brief Get exclusive access to global TWI resources.
 *
 * Wait to acquire bus hardware interface and ISR variables.
 *
 * \param no_wait  Set \c true to return instead of doing busy-wait (spin-lock).
 *
 * \return STATUS_OK if the bus is acquired, else ERR_BUSY.
 */
static inline status_code_t twim_acquire(bool no_wait)
{
	while (transfer.locked) {

		if (no_wait) { return ERR_BUSY; }
	}

	irqflags_t const flags = cpu_irq_save ();

	transfer.locked = true;
	transfer.status = OPERATION_IN_PROGRESS;

	cpu_irq_restore (flags);

	return STATUS_OK;
}

/**
 *@brief              TWI_MasterCalcBaud calculates the baud for the desired frequency
 *
 *@param              uint32_t frequency is the desired frequency
 *
 *@return             uint8_t value for the MBAUD register
 *@retval             the desired baud value
 */
#define TWI_BAUD(freq, t_rise) ((F_CPU / freq) / 2) - (5 + (((F_CPU / 1000000) * t_rise) / 2000))
uint8_t twim_calc_baud(uint32_t frequency)
{
  int16_t baud;

  #if (F_CPU == 20000000) || (F_CPU == 10000000)
    if (frequency >= 600000) {          // assuming 1.5kOhm
      baud = TWI_BAUD(frequency, 250);
    } else if (frequency >= 400000) {   // assuming 2.2kOhm
      baud = TWI_BAUD(frequency, 350);
    } else {                            // assuming 4.7kOhm
      baud = TWI_BAUD(frequency, 600);  // 300kHz will be off at 10MHz. Trade-off between size and accuracy
    }
  #else
    if (frequency >= 600000) {          // assuming 1.5kOhm
      baud = TWI_BAUD(frequency, 250);
    } else if (frequency >= 400000) {   // assuming 2.2kOhm
      baud = TWI_BAUD(frequency, 400);
    } else {                            // assuming 4.7kOhm
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

  if (baud < baudlimit) {
    return baudlimit;
  } else if (baud > 255) {
    return 255;
  }

  return (uint8_t)baud;
}

/**
 * \internal
 *
 * \brief Release exclusive access to global TWI resources.
 *
 * Release bus hardware interface and ISR variables previously locked by
 * a call to \ref twim_acquire().  This function will busy-wait for
 * pending driver operations to complete.
 *
 * \return  status_code_t
 *      - STATUS_OK if the transfer completes
 *      - ERR_BUSY to indicate an unavailable bus
 *      - ERR_IO_ERROR to indicate a bus transaction error
 *      - ERR_NO_MEMORY to indicate buffer errors
 *      - ERR_PROTOCOL to indicate an unexpected bus state
 */
status_code_t twim_release(void)
{
	/* timeout is used to get out of twim_release, when there is no device connected to the bus*/
	uint16_t timeout = 100;

	/* First wait for the driver event handler to indicate something
	 * other than a transfer in-progress, then test the bus interface
	 * for an Idle bus state.
	 */
	while (OPERATION_IN_PROGRESS == transfer.status);

	while ((! twim_idle(transfer.bus)) && --timeout) { barrier(); }

	status_code_t status = transfer.status;

	if(!timeout)
		status = ERR_TIMEOUT;

	transfer.locked = false;

	return status;
}

/**
 * \internal
 *
 * \brief TWI master write interrupt handler.
 *
 *  Handles TWI transactions (master write) and responses to (N)ACK.
 */
static inline void twim_write_handler(void)
{
	TWI_t * const         bus = transfer.bus;
	twi_package_t * const pkg = transfer.pkg;

	if (transfer.addr_count < pkg->addr_length) {

		const uint8_t * const data = pkg->addr;
		bus->MDATA = data[transfer.addr_count++];

	} else if (transfer.data_count < pkg->length) {

		if (transfer.read)
      {
			// Send repeated START condition (Address|R/W=1)
			bus->MADDR |= 0x01;

		}
      else
      {
			const uint8_t * const data = pkg->buffer;
			bus->MDATA = data[transfer.data_count++];
		}
	}
   else
   {
		// Send STOP condition to complete the transaction
		bus->MCTRLB = TWI_MCMD_STOP_gc;
		transfer.status = STATUS_OK;
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
static inline void twim_read_handler(void)
{
	TWI_t * const         bus = transfer.bus;
	twi_package_t * const pkg = transfer.pkg;

	if (transfer.data_count < pkg->length) {

		uint8_t * const data = pkg->buffer;
		data[transfer.data_count++] = bus->MDATA;

		/* If there is more to read, issue ACK and start a byte read.
		 * Otherwise, issue NACK and STOP to complete the transaction.
		 */
		if (transfer.data_count < pkg->length) {

			bus->MCTRLB = TWI_MCMD_RECVTRANS_gc;

		} else {

			bus->MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;
			transfer.status = STATUS_OK;
		}

	} else {

		/* Issue STOP and buffer overflow condition. */

		bus->MCTRLB = TWI_MCMD_STOP_gc;
		transfer.status = ERR_NO_MEMORY;
	}
}

/**
 * \internal
 *
 * \brief Common TWI master interrupt service routine.
 *
 *  Check current status and calls the appropriate handler.
 */
void twim_interrupt_handler(void)
{
	uint8_t const master_status = transfer.bus->MSTATUS;

	if (master_status & TWI_ARBLOST_bm) {

		transfer.bus->MSTATUS = master_status | TWI_ARBLOST_bm;
		transfer.bus->MCTRLB  = TWI_MCMD_STOP_gc;
		transfer.status = ERR_BUSY;

	} else if ((master_status & TWI_BUSERR_bm) ||
		(master_status & TWI_RXACK_bm)) {

		transfer.bus->MCTRLB = TWI_MCMD_STOP_gc;
		transfer.status = ERR_IO_ERROR;

	} else if (master_status & TWI_WIF_bm) {

		twim_write_handler();

	} else if (master_status & TWI_RIF_bm) {

		twim_read_handler();

	} else {

		transfer.status = ERR_PROTOCOL;
	}
}

/**
 * \brief Initialize the twi master module
 *
 * \param twi       Base address of the TWI (i.e. &TWIC).
 * \param *opt      Options for initializing the twi module
 *                  (see \ref twi_options_t)
 * \retval STATUS_OK        Transaction is successful
 * \retval ERR_INVALID_ARG  Invalid arguments in \c opt.
 */
status_code_t twi_master_init(TWI_t *twi)
{
   twi->MCTRLB |= TWI_FLUSH_bm;
	twi->MBAUD   = twim_calc_baud(TWI_SPEED);
	twi->MCTRLA  = TWI_RIEN_bm | TWI_WIEN_bm | TWI_ENABLE_bm;
	twi->MSTATUS = TWI_BUSSTATE_IDLE_gc;

	transfer.locked    = false;
	transfer.status    = STATUS_OK;

	return STATUS_OK;
}

/**
 * \brief Perform a TWI master write or read transfer.
 *
 * This function is a TWI Master write or read transaction.
 *
 * \param twi       Base address of the TWI (i.e. &TWI_t).
 * \param package   Package information and data
 *                  (see \ref twi_package_t)
 * \param read      Selects the transfer direction
 *
 * \return  status_code_t
 *      - STATUS_OK if the transfer completes
 *      - ERR_BUSY to indicate an unavailable bus
 *      - ERR_IO_ERROR to indicate a bus transaction error
 *      - ERR_NO_MEMORY to indicate buffer errors
 *      - ERR_PROTOCOL to indicate an unexpected bus state
 *      - ERR_INVALID_ARG to indicate invalid arguments.
 */
status_code_t twi_master_transfer(TWI_t *twi,
		const twi_package_t *package, bool read)
{
	/* Do a sanity check on the arguments. */

	if ((twi == NULL) || (package == NULL)) {
		return ERR_INVALID_ARG;
	}

	/* Initiate a transaction when the bus is ready. */

	status_code_t status = twim_acquire(package->no_wait);

	if (STATUS_OK == status) {
		transfer.bus         = (TWI_t *) twi;
		transfer.pkg         = (twi_package_t *) package;
		transfer.addr_count  = 0;
		transfer.data_count  = 0;
		transfer.read        = read;

		uint8_t const chip = (package->chip) << 1;

		if (package->addr_length || (false == read)) {
			transfer.bus->MADDR = chip;
		} else if (read) {
			transfer.bus->MADDR = chip | 0x01;
		}

		status = twim_release();
	}

	return status;
}
