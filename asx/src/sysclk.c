/**
 * \file
 *
 * \brief Chip-specific system clock management functions
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

#include <compiler.h>

#include <sysclk.h>
#include <osc.h>

void sysclk_init(void)
{
	/* Set up system clock prescalers if different from defaults */
	sysclk_set_prescalers(CONFIG_SYSCLK_PSDIV);

	/*
	 * Switch to the selected initial system clock source, unless
	 * the default internal 2 MHz oscillator is selected.
	 */
	if (CONFIG_SYSCLK_SOURCE != SYSCLK_SRC_RC20MHZ) {
		ccp_write_io((uint8_t *)&CLKCTRL.MCLKLOCK, CONFIG_SYSCLK_SOURCE);
      
      // Wait for the clock to stabilize
      switch (CONFIG_SYSCLK_SOURCE) {
      case SYSCLK_SRC_RC20MHZ:
         osc_wait_ready(OSC_ID_RC20MHZ);
         break;
      case SYSCLK_SRC_ULP32KHZ:
         osc_wait_ready(OSC_ID_ULP32KHZ);
         break;
      case SYSCLK_SRC_X32KHZ:
         osc_wait_ready(OSC_ID_X32KHZ);
         break;
      case SYSCLK_SRC_XOSC:
         osc_wait_ready(OSC_ID_XOSC);
         break;
      }
   }
}
