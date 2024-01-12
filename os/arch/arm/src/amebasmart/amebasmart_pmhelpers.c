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

static RTIM_TimeBaseInitTypeDef TIM_InitStruct_GT[8];
extern struct timer_s g_timer_wakeup;

void pg_timer_int_handler(void *Data)
{
	pmvdbg("Timer wakeup interrupt handler!!\n");
	RTIM_TimeBaseInitTypeDef *TIM_InitStruct = (RTIM_TimeBaseInitTypeDef *) Data;
	RTIM_INTClear(TIMx[TIM_InitStruct->TIM_Idx]);
	RTIM_Cmd(TIMx[TIM_InitStruct->TIM_Idx], DISABLE);
	// Reset the global struct
    g_timer_wakeup.use_timer = 0;
    g_timer_wakeup.timer_interval = 0;
	// Switch status back to normal mode after wake up from interrupt
	pm_activity(PM_IDLE_DOMAIN, 9);
}

void up_set_timer_interrupt(u32 TimerIdx, u32 Timercnt) {
	RTIM_TimeBaseInitTypeDef *pTIM_InitStruct_temp = &TIM_InitStruct_GT[TimerIdx];
	RCC_PeriphClockCmd(APBPeriph_TIM1, APBPeriph_TIM1_CLOCK, ENABLE);

	RTIM_TimeBaseStructInit(pTIM_InitStruct_temp);

	pTIM_InitStruct_temp->TIM_Idx = TimerIdx;
	pTIM_InitStruct_temp->TIM_Prescaler = 0x00;
	pTIM_InitStruct_temp->TIM_Period = 32768 * Timercnt - 1;//0xFFFF>>11;

	pTIM_InitStruct_temp->TIM_UpdateEvent = ENABLE; /* UEV enable */
	pTIM_InitStruct_temp->TIM_UpdateSource = TIM_UpdateSource_Overflow;
	pTIM_InitStruct_temp->TIM_ARRProtection = ENABLE;

	RTIM_TimeBaseInit(TIMx[TimerIdx], pTIM_InitStruct_temp, TIMx_irq[TimerIdx], (IRQ_FUN) pg_timer_int_handler,
						(u32)pTIM_InitStruct_temp);
	RTIM_INTConfig(TIMx[TimerIdx], TIM_IT_Update, ENABLE);
	RTIM_Cmd(TIMx[TimerIdx], ENABLE);

	SOCPS_SetAPWakeEvent_MSK0((WAKE_SRC_Timer1 << TimerIdx), ENABLE);
}

void ap_timer_helper(void) {
	// Check whether timer interrupt need to be set
	if (g_timer_wakeup.use_timer) {
		up_set_timer_interrupt(1, g_timer_wakeup.timer_interval);
	}
}

#endif