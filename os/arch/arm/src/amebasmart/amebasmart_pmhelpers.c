/****************************************************************************
 *
 * Copyright 2024 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * os/arch/arm/src/amebasmart/amebasmart_pmhelpers.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>
#include <tinyara/arch.h>
#include <tinyara/irq.h>

#ifdef CONFIG_PM
#include <tinyara/pm/pm.h>
#include "amebasmart_config.h"
#include "arch_timer.h"
#include "ameba_soc.h"
#include "osdep_service.h"
#include "timer_api.h"

/* TODO: One for periodical, One for one-shot */
gtimer_t g_timer1; //, g_timer2;
#ifdef CONFIG_DEBUG_PM_INFO
static uint32_t periodical_counter = 1;
#endif
extern struct timer_s g_timer_wakeup;

void SOCPS_SetAPWakeEvent_MSK0(u32 Option, u32 NewStatus)
{
	u32 WakeEvent = 0;

	/* Set Event */
	WakeEvent = HAL_READ32(PMC_BASE, WAK_MASK0_AP);
	if (NewStatus == ENABLE) {
		WakeEvent |= Option;
	} else {
		WakeEvent &= ~Option;
	}
	HAL_WRITE32(PMC_BASE, WAK_MASK0_AP, WakeEvent);
}

/**
  * @brief  set ap wake up event mask1.
  * @param  Option:
  *   This parameter can be any combination of the following values:
  *		 @arg WAKE_SRC_XXX
  * @param  NewStatus: TRUE/FALSE.
  * @retval None
  */
void SOCPS_SetAPWakeEvent_MSK1(u32 Option, u32 NewStatus)
{
	u32 WakeEvent = 0;

	/* Set Event */
	WakeEvent = HAL_READ32(PMC_BASE, WAK_MASK1_AP);
	if (NewStatus == ENABLE) {
		WakeEvent |= Option;
	} else {
		WakeEvent &= ~Option;
	}
	HAL_WRITE32(PMC_BASE, WAK_MASK1_AP, WakeEvent);
}

/**
  * @brief  get aon wake reason.
  * @param  None
  * @retval aon wake up reason
  *		This parameter can be one or combination of the following values:
  *		 @arg AON_BIT_RTC_ISR_EVT
  *		 @arg AON_BIT_GPIO_PIN3_WAKDET_EVT
  *		 @arg AON_BIT_GPIO_PIN2_WAKDET_EVT
  *		 @arg AON_BIT_GPIO_PIN1_WAKDET_EVT
  *		 @arg AON_BIT_GPIO_PIN0_WAKDET_EVT
  *		 @arg AON_BIT_TIM_ISR_EVT
  *		 @arg AON_BIT_BOR_ISR_EVT
  *		 @arg AON_BIT_CHIPEN_LP_ISR_EVT
  *		 @arg AON_BIT_CHIPEN_SP_ISR_EVT
  */
int SOCPS_AONWakeReason(void)
{
	int reason = 0;

	reason = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_AON_WAK_EVT);

	return reason;
}

void one_shot_timer_handler(void *Data)
{
	pmvdbg("One Shot timer wakeup interrupt handler!!\n");
	// Reset the global struct
	g_timer_wakeup.is_periodical = 0;
    g_timer_wakeup.timer_interval = 0;
	// Switch status back to normal mode after wake up from interrupt
	pm_activity(PM_IDLE_DOMAIN, 9);
}

void periodical_timer_handler(void *Data)
{
#ifdef CONFIG_DEBUG_PM_INFO
	pmvdbg("Periodical timer wakeup interrupt handler, count = %d!!\n", periodical_counter);
	periodical_counter += 1;
#endif
	// Switch status back to normal mode after wake up from interrupt
	pm_activity(PM_IDLE_DOMAIN, 9);
	/* Anything else Samsung want to do here? */
	/*
	
	*/
}

void ap_timer_helper(void) 
{
	// Check whether timer interrupt need to be set
	/* Note: 2 Separate Timer O and P can be initialized at the same time
	Timer O: Is initialized with smallest one shot duration out of all timer request
	Timer P: Is initialized with smallet periodical duration out of all timer request
	Q1 (Both One-shot): App1 set timer for 10 secs, App2 set timer for 3 secs, board wake up after 3 secs, does some work and go back to sleep at 7 sec mark
	now the board is sleeping, and App1 assume a wakeup at 10 sec mark, but this will fail to trigger
	Q2 (1 One-shot, 1 Periodical): O = 7secs, P = 5secs, the board woken up at 5 sec mark, but O timer will trigger after 2 sec (ie. 7 sec mark), wastage handler
	Q2.b: O = 3secs, P = 5secs, handler happens at-> 3, 5, 10 sec mark
	Q2.c: O = 3secs, P = 10secs, board wakes up at 3sec mark, goes to sleep at 8sec mark, woken up again at 10sec mark (this will cause an issue, which the power efficiency will be low)
	*/
	if (g_timer_wakeup.timer_interval > 0) {
		gtimer_init(&g_timer1, TIMER1);
		g_timer_wakeup.is_periodical == 1 ? gtimer_start_periodical(&g_timer1, g_timer_wakeup.timer_interval, (void *)periodical_timer_handler, NULL) :
											gtimer_start_one_shout(&g_timer1, g_timer_wakeup.timer_interval, (void *)one_shot_timer_handler, NULL);
		g_timer_wakeup.timer_interval = 0;
	}
	else {
		gtimer_stop(&g_timer1);
	}
}

/* Interrupt callback from wifi-keepalive, which LP received designated packet*/
void SOCPS_LPWAP_ipc_int(VOID *Data, u32 IrqStatus, u32 ChanNum)
{
	/* To avoid gcc warnings */
	UNUSED(Data);
	UNUSED(IrqStatus);
	UNUSED(ChanNum);

	pmvdbg("IPC wakeup interrupt handler!!\n");
	pm_activity(PM_IDLE_DOMAIN, 9);
	ipc_get_message(IPC_LP_TO_AP, IPC_L2A_Channel1);

}

IPC_TABLE_DATA_SECTION

const IPC_INIT_TABLE ipc_LPWHP_table[] = {
	{
		.USER_MSG_TYPE = IPC_USER_DATA,
		.Rxfunc = SOCPS_LPWAP_ipc_int,
		.RxIrqData = (VOID *) NULL,
		.Txfunc = IPC_TXHandler,
		.TxIrqData = (VOID *) NULL,
		.IPC_Direction = IPC_LP_TO_AP,
		.IPC_Channel = IPC_L2A_Channel1
	},

};

#endif