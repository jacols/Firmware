/****************************************************************************
 *
 *   Copyright (c) 2015 PX4 Development Team. All rights reserved.
 *       Author: Ben Dyer <ben_dyer@mac.com>
 *               Pavel Kirienko <pavel.kirienko@zubax.com>
 *               David Sidrane <david_s5@nscdg.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "boot_config.h"
#include "board.h"

#include <debug.h>
#include <string.h>
#include <arch/board/board.h>

#include <nuttx/board.h>
#include "led.h"


/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/************************************************************************************
 * Name: stm32_boardinitialize
 *
 * Description:
 *   All STM32 architectures must provide the following entry point.  This entry point
 *   is called early in the initialization -- after all memory has been configured
 *   and mapped but before any devices have been initialized.
 *
 ************************************************************************************/

__EXPORT void stm32_boardinitialize(void)
{
	putreg32(getreg32(STM32_RCC_APB1ENR) | RCC_APB1ENR_CAN1EN, STM32_RCC_APB1ENR);
	stm32_configgpio(GPIO_CAN1_RX);
	stm32_configgpio(GPIO_CAN1_TX);
	stm32_configgpio(GPIO_CAN_CTRL);
	putreg32(getreg32(STM32_RCC_APB1RSTR) | RCC_APB1RSTR_CAN1RST,
		 STM32_RCC_APB1RSTR);
	putreg32(getreg32(STM32_RCC_APB1RSTR) & ~RCC_APB1RSTR_CAN1RST,
		 STM32_RCC_APB1RSTR);

#if defined(OPT_WAIT_FOR_GETNODEINFO_JUMPER_GPIO)
	stm32_configgpio(GPIO_GETNODEINFO_JUMPER);
#endif

}

/************************************************************************************
 * Name: stm32_boarddeinitialize
 *
 * Description:
 *   This function is called by the bootloader code prior to booting
 *   the application. Is should place the HW into an benign initialized state.
 *
 ************************************************************************************/

void stm32_boarddeinitialize(void)
{

	putreg32(getreg32(STM32_RCC_APB1RSTR) | RCC_APB1RSTR_CAN1RST,
		 STM32_RCC_APB1RSTR);
}

/****************************************************************************
 * Name: board_get_product_name
 *
 * Description:
 *   Called to retrieve the product name. The returned value is a assumed
 *   to be written to a pascal style string that will be length prefixed
 *   and not null terminated
 *
 * Input Parameters:
 *    product_name - A pointer to a buffer to write the name.
 *    maxlen       - The maximum number of charter that can be written
 *
 * Returned Value:
 *   The length of characters written to the buffer.
 *
 ****************************************************************************/

uint8_t board_get_product_name(uint8_t *product_name, size_t maxlen)
{
	DEBUGASSERT(maxlen > UAVCAN_STRLEN(UAVCANBL_NAME));
	memcpy(product_name, UAVCANBL_NAME, UAVCAN_STRLEN(UAVCANBL_NAME));
	return UAVCAN_STRLEN(UAVCANBL_NAME);
}

/****************************************************************************
 * Name: board_get_hardware_version
 *
 * Description:
 *   Called to retrive the hardware version information.
 *
 * Input Parameters:
 *    hw_version - A pointer to a uavcan_hardwareversion_t.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void board_get_hardware_version(uavcan_hardwareversion_t *hw_version)
{
	uint32_t i;
	volatile uint8_t *stm32f_uid = (volatile uint8_t *)STM32_SYSMEM_UID;

	hw_version->major = 1u;
	hw_version->minor = 0u;

	for (i = 0u; i < 12u; i++) {
		hw_version->unique_id[i] = stm32f_uid[i];
	}

	for (; i < 16u; i++) {
		hw_version->unique_id[i] = 0u;
	}

	for (i = 0u; i < 255u; i++) {
		hw_version->certificate_of_authenticity[i] = 0;
	}

	hw_version->certificate_of_authenticity_length = 0u;
}

/****************************************************************************
 * Name: board_indicate
 *
 * Description:
 *   Provides User feedback to indicate the state of the bootloader
 *   on board specific  hardware.
 *
 * Input Parameters:
 *    indication - A member of the uiindication_t
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/
#define led(n, code, r , g , b, h) {.red = (r),.green = (g), .blue = (b),.hz = (h)}

typedef struct led_t {
	int red;
	int green;
	int blue;
	int hz;
} led_t;

static const  led_t i2l[] = {

	led(0, off,                            0   , 0   , 0   , 0),
	led(1, reset,                          10  , 255 , 255 , 1000),
	led(2, autobaud_start,                 0   , 255 , 0   , 1),
	led(3, autobaud_end,                   0   , 255 , 0   , 2),
	led(4, allocation_start,               0   , 0   , 255 , 2),
	led(5, allocation_end,                 0   , 255 , 255 , 3),
	led(6, fw_update_start,                100 , 255 , 255 , 3),
	led(7, fw_update_erase_fail,           100 , 255 , 100 , 3),
	led(8, fw_update_invalid_response,     255 , 0   , 0   , 1),
	led(9, fw_update_timeout,              255 , 0   , 0   , 2),
	led(a, fw_update_invalid_crc,          255 , 0   , 0   , 4),
	led(b, jump_to_app,                    0   , 255 , 0   , 10),

};

void board_indicate(uiindication_t indication)
{
	rgb_led(i2l[indication].red,
		i2l[indication].green,
		i2l[indication].blue,
		i2l[indication].hz);
}
