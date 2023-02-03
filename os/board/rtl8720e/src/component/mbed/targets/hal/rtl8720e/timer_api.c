/** mbed Microcontroller Library
 ******************************************************************************
 * @file    timer_api.c
 * @author
 * @version V1.0.0
 * @date    2016-08-01
 * @brief   This file provides mbed API for gtimer.
 ******************************************************************************
 * @attention
 *
 * This module is a confidential and proprietary property of RealTek and
 * possession or use of this module requires written permission of RealTek.
 *
 * Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved.
 ******************************************************************************
 */

#include "objects.h"
#include "timer_api.h"

/** @addtogroup Ameba_Mbed_API
  * @{
  */

/** @defgroup MBED_TIMER
 *  @brief    MBED_TIMER driver modules.
 *  @{
 */

/** @defgroup MBED_TIMER_Exported_Functions MBED_TIMER Exported Functions
  * @{
  */

/**
  * @brief  Register gtimer interrupt handler.
  * @param  data: Gtimer IRQ callback parameter.
  * @retval none
  */
static u32 gtimer_timeout_handler(void *data)
{
	printf("gtimer_timeout_handler reached\n");
	gtimer_t *obj = (gtimer_t *)data;
	uint32_t tid = obj->timer_id;
	gtimer_irq_handler handler;

	RTIM_INTClear(TIMx[tid]);

	if (obj->handler != NULL) {
		handler = (gtimer_irq_handler)obj->handler;
		handler(obj->hid);
	}

	if (!obj->is_periodcal) {
		gtimer_stop(obj);
	}

	return 0;
}

/**
  * @brief  Initialize the timer device, including timer registers and interrupt.
  * @param  obj: Timer object defined in application software.
  * @param  tid: General timer ID, which can be one of the following parameters:
  *     @arg TIMER0
  *     @arg TIMER1
  *     @arg TIMER2
  *     @arg TIMER3
  *     @arg TIMER4
  *     @arg TIMER5
  *     @arg TIMER6
  *     @arg TIMER7
  *     @arg TIMER8
  *     @arg TIMER9
  *     @arg TIMER10
  *     @arg TIMER11
  *     @arg TIMER12
  *     @arg TIMER13
  *     @arg TIMER14
  * @note KM4 TIMER0/1/2/3/4/5/6/7/8/9/10/11/12/13/14 are recommended.
  * @retval none
  */
void INTClear(RTIM_TypeDef *TIMx)
{
	u32 CounterIndex = 0;
	u32 Status = TIMx->SR;

	/* Check the parameters */
	assert_param(IS_TIM_ALL_TIM(TIMx));

	/* Clear the all IT pending Bits */
	TIMx->SR = Status;

	/* make sure write ok, because bus delay */
	while (1) {
		CounterIndex++;
		if (CounterIndex >= 300) {
			//printf("function %s line %d \n", __FUNCTION__, __LINE__);
			break;
		}

		if (((TIMx->SR) & 0xFFFFFF) == 0) {
			//printf("function %s line %d \n", __FUNCTION__, __LINE__);
			break;
		}
	}
}

u32 GetINTStatus(RTIM_TypeDef *TIMx, u32 TIM_IT)
{
	u32 bitstatus = _FALSE;
	u32 itstatus = 0x0, itenable = 0x0;

	/* Check the parameters */
	assert_param(IS_TIM_ALL_TIM(TIMx));
	assert_param(IS_TIM_GET_IT(TIM_IT));

	itstatus = TIMx->SR & TIM_IT;
	itenable = TIMx->DIER & TIM_IT;
	printf("function %s line %d TIMx->SR %d TIMx->DIER %x TIM_IT %x \n", __FUNCTION__, __LINE__,TIMx->SR, TIMx->DIER, TIM_IT);

	if ((itstatus != 0) && (itenable != 0)) {
		bitstatus = _TRUE;
	} else {
		bitstatus = _FALSE;
	}

	return bitstatus;
}

u32 pg_timer_int_handler_2(void *Data)
{
	RTIM_TimeBaseInitTypeDef *TIM_InitStruct = (RTIM_TimeBaseInitTypeDef *) Data;
	RTIM_INTClear(TIMx[TIM_InitStruct->TIM_Idx]);
	RTIM_Cmd(TIMx[TIM_InitStruct->TIM_Idx], DISABLE);
//	uint32_t int_status = GetINTStatus(TIMx[TIM_InitStruct->TIM_Idx], TIM_IT_Update);
//	printf("function %s line %d timer %x int_status %d\n", __FUNCTION__, __LINE__, TIM_InitStruct->TIM_Idx, int_status);
	// RTIM_INTClear(TIMx[TIM_InitStruct->TIM_Idx]);
	// RTIM_Cmd(TIMx[TIM_InitStruct->TIM_Idx], DISABLE);
//	INTClear(TIMx[7]);
//	RTIM_INTConfig(TIMx[7], TIM_IT_Update, DISABLE);
//	RTIM_Cmd(TIMx[7], DISABLE);

	int n = sizeof(TIMx)/sizeof(TIMx);
	printf("function %s line %d TIMx size %d\n", __FUNCTION__, __LINE__, n);

	// InterruptDis(TIMx_irq[TIM_InitStruct->TIM_Idx]);
//	InterruptUnRegister(23); //TIMx_irq[7]);

	printf("function %s line %d timer %x\n", __FUNCTION__, __LINE__, TIM_InitStruct);

//	int_status = GetINTStatus(TIMx[TIM_InitStruct->TIM_Idx], TIM_IT_Update);
	//printf("function %s line %d timer %x int_status %d\n", __FUNCTION__, __LINE__, TIM_InitStruct->TIM_Idx, int_status);

	return 0;
}
RTIM_TimeBaseInitTypeDef TIM_InitStruct_GT[8];

extern SLEEP_ParamDef sleep_param;
void gtimer_init_2(gtimer_t *obj, uint32_t tid)
{
	u32 KR4_is_NP = 0;
	KR4_is_NP = LSYS_GET_KR4_IS_NP(HAL_READ32(SYSTEM_CTRL_BASE, REG_LSYS_SYSTEM_CFG1));
	sleep_param.sleep_type = SLEEP_PG;
	sleep_param.sleep_time = 0;
	sleep_param.dlps_enable = 0;
	// SOCPS_sleepInit();

	RTIM_TimeBaseInitTypeDef *TIM_InitStruct = &TIM_InitStruct_GT[tid];
	
	assert_param(tid < GTIMER_MAX);
	RCC_PeriphClockCmd(APBPeriph_TIMx[tid], APBPeriph_TIMx_CLOCK[tid], ENABLE);

	obj->timer_id = tid;

	RTIM_TimeBaseStructInit(TIM_InitStruct);
	TIM_InitStruct->TIM_Idx = (u8)tid;
	TIM_InitStruct->TIM_Prescaler = 0x10;
	TIM_InitStruct->TIM_Period = 32768 * 20;
	TIM_InitStruct->TIM_UpdateEvent = ENABLE; /* UEV enable */
	TIM_InitStruct->TIM_UpdateSource = TIM_UpdateSource_Overflow;
	TIM_InitStruct->TIM_ARRProtection = ENABLE;
	//RTIM_TimeBaseInit(TIMx[tid], &TIM_InitStruct, TIMx_irq[tid], (IRQ_FUN) gtimer_timeout_handler, (u32)obj);
	printf("function %s line %d TIM_InitStruct %x\n", __FUNCTION__, __LINE__, &TIM_InitStruct);

 	printf("function %s line %d \n TIM_Prescaler %x\n TIM_Period %x\n TIM_UpdateEvent %x\n TIM_UpdateSource	%x\n TIM_ARRProtection %x\nTIM_Idx %d\nTIM_SecureTimer %x\n",
			__FUNCTION__, __LINE__,
			TIM_InitStruct->TIM_Prescaler, 
			TIM_InitStruct->TIM_Period, 
			TIM_InitStruct->TIM_UpdateEvent, 
			TIM_InitStruct->TIM_UpdateSource, 
			TIM_InitStruct->TIM_ARRProtection, 
			TIM_InitStruct->TIM_Idx, 
			TIM_InitStruct->TIM_SecureTimer);


	printf("F %s L %d TIMx %d TIMx_irq %d\n", __FUNCTION__, __LINE__,TIMx[tid],TIMx_irq[tid]);
	// RTIM_TimeBaseInit(TIMx[tid], TIM_InitStruct, TIMx_irq[tid], NULL, NULL);
	RTIM_TimeBaseInit(TIMx[tid], TIM_InitStruct, TIMx_irq[tid], (IRQ_FUN) gtimer_timeout_handler, (u32)obj);
	
	InterruptRegister(pg_timer_int_handler_2, TIMx_irq[tid], (void *)TIM_InitStruct, 4);
	InterruptEn(TIMx_irq[tid], 4);

	RTIM_INTConfig(TIMx[tid], TIM_IT_Update, ENABLE);
	RTIM_Cmd(TIMx[tid], ENABLE);

	// printf("F %s L %d KR4_is_NP %llu\n", __FUNCTION__, __LINE__,KR4_is_NP);
	if (KR4_is_NP) {
		// printf("\aaaaaaa\n");
		SOCPS_SetAPWakeEvent_MSK0(WAKE_SRC_Timer1 << tid, ENABLE);
	} else {
		// printf("\bbbbbbb\n");
		SOCPS_SetNPWakeEvent_MSK0(WAKE_SRC_Timer1 << tid, ENABLE);
	}
}

void gtimer_init(gtimer_t *obj, uint32_t tid)
{
	RTIM_TimeBaseInitTypeDef TIM_InitStruct;

	assert_param(tid < GTIMER_MAX);
	RCC_PeriphClockCmd(APBPeriph_TIMx[tid], APBPeriph_TIMx_CLOCK[tid], ENABLE);

	obj->timer_id = tid;

	RTIM_TimeBaseStructInit(&TIM_InitStruct);
	TIM_InitStruct.TIM_Idx = (u8)tid;

	TIM_InitStruct.TIM_UpdateEvent = ENABLE; /* UEV enable */
	TIM_InitStruct.TIM_UpdateSource = TIM_UpdateSource_Overflow;
	TIM_InitStruct.TIM_ARRProtection = ENABLE;

	RTIM_TimeBaseInit(TIMx[tid], &TIM_InitStruct, TIMx_irq[tid], (IRQ_FUN) gtimer_timeout_handler, (u32)obj);
}

/**
  * @brief  Deinitialize the timer device, including interrupt and timer registers.
  * @param  obj: Timer object defined in application software.
  * @retval none
  */
void gtimer_deinit(gtimer_t *obj)
{
	uint32_t tid = obj->timer_id;

	assert_param(tid < GTIMER_MAX);

	RTIM_DeInit(TIMx[tid]);
}

/**
  * @brief  Get counter value of the specified timer.
  * @param  obj: Timer object defined in application software.
  * @return Counter value.
  */
uint32_t gtimer_read_tick(gtimer_t *obj)
{
	uint32_t tid = obj->timer_id;

	assert_param(tid < GTIMER_MAX);

	return (RTIM_GetCount(TIMx[tid]));
}

/**
  * @brief  Read current timer tick in microsecond.
  * @param  obj: Timer object defined in application software.
  * @return 64b tick time in microsecond(us).
  */
uint64_t gtimer_read_us(gtimer_t *obj)   //need to be test in IAR(64bit computing)
{
	assert_param((obj->timer_id < GTIMER_MAX) && (obj->timer_id != TIMER8) && (obj->timer_id != TIMER9));

	uint64_t time_us;
	if (obj->timer_id <= TIMER7) {
		time_us = (uint64_t)(gtimer_read_tick(obj) * ((float)1000000 / 32768));
	} else if (obj->timer_id >= TIMER10) {
		time_us = gtimer_read_tick(obj);
	}

	return (time_us);
}

/**
  * @brief  Change period of the specified timer.
  * @param  obj: Timer object defined in application software.
  * @param  duration_us: Period to be set in microseconds.
  * @retval none
  */
void gtimer_reload(gtimer_t *obj, uint32_t duration_us)
{
	uint32_t tid = obj->timer_id;
	uint32_t temp = 0;

	assert_param(obj->timer_id < GTIMER_MAX && (obj->timer_id != TIMER8) && (obj->timer_id != TIMER9));
	if (obj->timer_id <= TIMER7) {
		temp = (uint32_t)((float)duration_us / 1000000 * 32768) - 1;
	} else {
		temp = (uint32_t)duration_us - 1;
	}
	RTIM_ChangePeriodImmediate(TIMx[tid], temp);
}

/**
  * @brief  Start the specified timer and enable update interrupt.
  * @param  obj: Timer object defined in application software.
  * @retval none
  */
void gtimer_clear_interrupt(gtimer_t *obj){
	uint32_t tid = obj->timer_id;
	assert_param(tid < GTIMER_MAX);
	RTIM_INTConfig(TIMx[tid], TIM_IT_Update, DISABLE);

}
void gtimer_start(gtimer_t *obj)
{
	uint32_t tid = obj->timer_id;

	assert_param(tid < GTIMER_MAX);

	RTIM_INTConfig(TIMx[tid], TIM_IT_Update, ENABLE);
	RTIM_Cmd(TIMx[tid], ENABLE);
}

/**
  * @brief Start the specified timer in one-shot mode with specified period and interrupt handler.
  * @param  obj: Timer object defined in application software.
  * @param  duration_us: Period to be set in microseconds.
  * @param  handler: User-defined IRQ callback function.
  * @param  hid: User-defined IRQ callback parameter.
  * @retval none
  * @note In one-shot mode, timer will stop counting the first time counter overflows.
  */
void gtimer_start_one_shout(gtimer_t *obj, uint32_t duration_us, void *handler, uint32_t hid)
{
	assert_param(obj->timer_id < GTIMER_MAX);

	obj->is_periodcal = _FALSE;
	obj->handler = handler;
	obj->hid = hid;

	gtimer_reload(obj, duration_us);
	gtimer_start(obj);
}

/**
  * @brief Start the specified timer in periodical mode with specified period and interrupt handler.
  * @param  obj: Timer object defined in application software.
  * @param  duration_us: Period to be set in microseconds.
  * @param  handler: User-defined IRQ callback function.
  * @param  hid: User-defined IRQ callback parameter.
  * @retval none
  * @note In periodical mode, timer will restart from 0 each time the counter overflows.
  */
void gtimer_start_periodical(gtimer_t *obj, uint32_t duration_us, void *handler, uint32_t hid)
{
	assert_param(obj->timer_id < GTIMER_MAX);

	obj->is_periodcal = _TRUE;
	obj->handler = handler;
	obj->hid = hid;

	gtimer_reload(obj, duration_us);
	gtimer_start(obj);
}

/**
  * @brief  Disable the specified timer peripheral.
  * @param  obj: Timer object defined in application software.
  * @retval none
  */
void gtimer_stop(gtimer_t *obj)
{
	uint32_t tid = obj->timer_id;

	assert_param(tid < GTIMER_MAX);

	RTIM_Cmd(TIMx[tid], DISABLE);
}
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */