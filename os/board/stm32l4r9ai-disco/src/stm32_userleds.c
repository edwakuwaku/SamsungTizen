/****************************************************************************
 * configs/stm32l4r9ai-disco/src/stm32_userleds.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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
 * 3. Neither the name NuttX nor the names of its contributors may be
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

#include <tinyara/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <debug.h>

#include <arch/board/board.h>
#include <tinyara/pm/pm.h>

#include "chip.h"
#include "up_arch.h"
#include "up_internal.h"
#include "stm32l4.h"
#include "stm32l4r9ai-disco.h"

#ifndef CONFIG_ARCH_LEDS

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* LED Power Management */

#ifdef CONFIG_PM
static void led_pm_notify(struct pm_callback_s *cb,
                          enum pm_state_e pmstate);
static int led_pm_prepare(struct pm_callback_s *cb,
                          enum pm_state_e pmstate);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_PM
static struct pm_callback_s g_ledscb =
{
  .notify  = led_pm_notify,
  .prepare = led_pm_prepare,
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: led_pm_notify
 *
 * Description:
 *   Notify the driver of new power state. This callback is called after
 *   all drivers have had the opportunity to prepare for the new power state.
 *
 ****************************************************************************/

#ifdef CONFIG_PM
static void led_pm_notify(struct pm_callback_s *cb,
                          enum pm_state_e pmstate)
{
  switch (pmstate)
    {
      case(PM_NORMAL):
        {
          /* Restore normal LEDs operation */

          //stm32l4_gpiowrite(GPIO_LED_RED, (ledset & BOARD_LED_RED_BIT) != 0);
          //stm32l4_gpiowrite(GPIO_LED_GRN, (ledset & BOARD_LED_GRN_BIT) != 0);
        }
        break;

      case(PM_IDLE):
        {
          /* Entering IDLE mode - Turn leds off */

          stm32l4_gpiowrite(GPIO_LED_RED, 0);
          stm32l4_gpiowrite(GPIO_LED_GRN, 0);
        }
        break;

      case(PM_STANDBY):
        {
          /* Entering STANDBY mode - Logic for PM_STANDBY goes here */

          stm32l4_gpiowrite(GPIO_LED_RED, 0);
          stm32l4_gpiowrite(GPIO_LED_GRN, 0);
        }
        break;

      case(PM_SLEEP):
        {
          /* Entering SLEEP mode - Logic for PM_SLEEP goes here */

          stm32l4_gpiowrite(GPIO_LED_RED, 0);
          stm32l4_gpiowrite(GPIO_LED_GRN, 0);
        }
        break;

      default:
        {
          /* Should not get here */
        }
        break;
    }
}
#endif

/****************************************************************************
 * Name: led_pm_prepare
 *
 * Description:
 *   Request the driver to prepare for a new power state. This is a warning
 *   that the system is about to enter into a new power state. The driver
 *   should begin whatever operations that may be required to enter power
 *   state. The driver may abort the state change mode by returning a
 *   non-zero value from the callback function.
 *
 ****************************************************************************/

#ifdef CONFIG_PM
static int led_pm_prepare(struct pm_callback_s *cb,
                          enum pm_state_e pmstate)
{
  /* No preparation to change power modes is required by the LEDs driver.
   * We always accept the state change by returning OK.
   */

  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_userled_initialize
 ****************************************************************************/

void board_userled_initialize(void)
{
  /* Configure LD4,5 GPIO for output */

  stm32l4_configgpio(GPIO_LED_RED);
  stm32l4_configgpio(GPIO_LED_GRN);
}

/****************************************************************************
 * Name: board_userled
 ****************************************************************************/

void board_userled(int led, bool ledon)
{
  switch (led)
  {
  case BOARD_LED_RED:
    stm32l4_gpiowrite(GPIO_LED_RED, ledon);
    break;

  case BOARD_LED_GRN:
    stm32l4_gpiowrite(GPIO_LED_GRN, ledon);
    break;
  }
}

/****************************************************************************
 * Name: board_userled_all
 ****************************************************************************/

void board_userled_all(uint8_t ledset)
{
  stm32l4_gpiowrite(GPIO_LED_RED, (ledset & BOARD_LED_RED_BIT) != 0);
  stm32l4_gpiowrite(GPIO_LED_GRN, (ledset & BOARD_LED_GRN_BIT) != 0);
}

/****************************************************************************
 * Name: stm32_led_pminitialize
 ****************************************************************************/

#ifdef CONFIG_PM
void stm32_led_pminitialize(void)
{
  /* Register to receive power management callbacks */

  int ret = pm_register(&g_ledscb);
  DEBUGASSERT(ret == OK);
  UNUSED(ret);
}
#endif /* CONFIG_PM */

#endif /* !CONFIG_ARCH_LEDS */
