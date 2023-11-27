/****************************************************************************
 * arch/arm/src/amebasmart/amebasmart_pmhelpers.c
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
#include "arm_internal.h"

#ifdef CONFIG_PM
#include <tinyara/pm/pm.h>
#include "amebasmart_config.h"
#include "arch_timer.h"
#include "ameba_soc.h"
#include "osdep_service.h"

static bool system_np_wakelock = 1;
static RTIM_TimeBaseInitTypeDef TIM_InitStruct_GT[8];
extern struct timer_s g_timer_wakeup;

struct task_struct np_wakelock_release_handler;
extern void rtk_NP_powersave_enable(void);
static void np_wakelock_release(void) {
	if (rtw_create_task(&np_wakelock_release_handler, (const char *const)"rtk_NP_powersave_enable_task", 512, 3, (void*)rtk_NP_powersave_enable, NULL) != 1) {
		DiagPrintf("Create np_wakelock_release_handler Err!!\n");
	}
}
extern void rtk_NP_powersave_disable(void);
struct task_struct np_wakelock_acquire_handler;
static void np_wakelock_acquire(void) {
	if (rtw_create_task(&np_wakelock_acquire_handler, (const char *const)"rtk_NP_powersave_disable_task", 512, 3, (void*)rtk_NP_powersave_disable, NULL) != 1) {
		DiagPrintf("Create np_wakelock_acquire_handler Err!!\n");
	}
}

void pg_timer_int_handler(void *Data)
{
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
	// Application might invoke state directly to PM_SLEEP, so need to check whether np wakelock has already been released
	if (system_np_wakelock) {
		np_wakelock_release();
		rtw_delete_task(&np_wakelock_release_handler);
		system_np_wakelock = 0;
	}
	// Check whether timer interrupt need to be set
	if (g_timer_wakeup.use_timer) {
		up_set_timer_interrupt(1, g_timer_wakeup.timer_interval);
	}
}


void np_wakelock_helper(void) {
	if(system_np_wakelock) {
		// IPN AP->NP to release wakelock
		np_wakelock_release();
		rtw_delete_task(&np_wakelock_release_handler);
		system_np_wakelock = 0;
	}
	else {
		// IPC AP->NP to acquire wakelock
		np_wakelock_acquire();
		rtw_delete_task(&np_wakelock_acquire_handler);
		system_np_wakelock = 1;
	}
}
#endif
