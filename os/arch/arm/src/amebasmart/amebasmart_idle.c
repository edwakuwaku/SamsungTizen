/****************************************************************************
 * arch/arm/src/amebasmart/amebasmart_idle.c
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
#endif

#ifdef CONFIG_SMP
#include "gic.h"
#endif

// static u32 system_can_yield = 1;
/****************************************************************************
 * Name: up_idlepm
 *
 * Description:
 *   Perform IDLE state power management.
 *
 ****************************************************************************/

static enum pm_state_e oldstate = PM_NORMAL;
#ifdef CONFIG_PM
static void up_idlepm(void)
{
	uint32_t xModifiableIdleTime = 0;
	enum pm_state_e newstate;
	irqstate_t flags;
	int ret;

	/* Decide, which power saving level can be obtained */
	/* If up_idlepm() need to be callable from pm_ioctl, this part
	   should be revised?
	*/
	newstate = pm_checkstate(PM_IDLE_DOMAIN);

	/* Check for state changes */
	if (newstate != oldstate)
	{
		//TODO: Critical section code needed for SMP case?
		//Any additional implications of putting a core in critical section while trying to sleep?
		/* Perform board-specific, state-dependent logic here */
	  	pmvdbg("newstate= %d oldstate=%d\n", newstate, oldstate);

		/* Then force the global state change */
		ret = pm_changestate(PM_IDLE_DOMAIN, newstate);
		if (ret < 0) {
			/* The new state change failed, revert to the preceding state */
			pmdbg("State change failed! Current state = %d, newstate = %d\n", oldstate, newstate);
			newstate = oldstate;
			goto EXIT2;
		} else {
			/* Save the new state */
			oldstate = newstate;
		}
		/* MCU-specific power management logic */
		switch (newstate) {
			case PM_NORMAL:
			case PM_IDLE:
				pmvdbg("\n[%s] - %d, state = %d\n",__FUNCTION__,__LINE__, newstate);
				break;
			case PM_STANDBY:
				pmvdbg("\n[%s] - %d, state = %d\n",__FUNCTION__,__LINE__, newstate);
				np_wakelock_helper();
				break;
			case PM_SLEEP:
				pmvdbg("\n[%s] - %d, state = %d\n",__FUNCTION__,__LINE__, newstate);
				/* need further check, for SMP case*/
				// system_can_yield = 0;
				ap_timer_helper();
				if (up_cpu_index() == 0) {
					/* mask sys tick interrupt*/
					arm_arch_timer_int_mask(1);
					up_timer_disable();
					flags = irqsave();
					if (tizenrt_ready_to_sleep()) {
// Consider for dual core condition
#ifdef CONFIG_SMP
						/*PG flow */
						if (pmu_get_sleep_type() == SLEEP_PG) {
							/* CPU1 just come back from pg, so can't sleep here */
							if (pmu_get_secondary_cpu_state(1) == CPU1_WAKE_FROM_PG) {
								goto EXIT;
							}

							/* CPU1 is in task schedular, tell CPU1 to enter hotplug */
							if (pmu_get_secondary_cpu_state(1) == CPU1_RUNNING) {
								/* CPU1 may in WFI idle state. Wake it up to enter hotplug itself */
								up_irq_enable();
								arm_gic_raise_softirq(1, 0);
								arm_arch_timer_int_mask(0);
								DelayUs(100);
								goto EXIT;
							}
							/* CG flow */
						} else {
							if (!check_wfi_state(1)) {
								goto EXIT;
							}
						}
#endif
						// Interrupt source from BT/UART will wake cpu up, just leave expected idle time as 0
						// Enter sleep mode for AP
						configPRE_SLEEP_PROCESSING(xModifiableIdleTime);
						/* When wake from pg, arm timer has been reset, so a new compare value is necessary to
						trigger an timer interrupt */
						if (pmu_get_sleep_type() == SLEEP_PG) {
							up_timer_enable();
							arm_arch_timer_set_compare(arm_arch_timer_count() + 50000);
						}
						arm_arch_timer_int_mask(0);
						configPOST_SLEEP_PROCESSING(xModifiableIdleTime);
					}
					else {
						/* power saving when idle*/
						arm_arch_timer_int_mask(0);
						__asm(" DSB");
						__asm(" WFI");
						__asm(" ISB");
					}
#ifdef CONFIG_SMP
EXIT:
#endif				
					/* Re-enable interrupts and sys tick*/
					up_irq_enable();
				}
				// This case is consideration for secondary core
				else if (up_cpu_index() == 1) {
					if (pmu_get_sleep_type() == SLEEP_PG) {
						if (tizenrt_ready_to_sleep()) {
							/* CPU1 will enter hotplug state. Raise a task yield to migrate its task */
							pmu_set_secondary_cpu_state(1, CPU1_HOTPLUG);
							// Check portYIELD();
							portYIELD();
						}
					}

					flags = irqsave();
					__asm("	DSB");
					__asm("	WFI");
					__asm("	ISB");
					up_irq_enable();
				}
				/* need further check*/
				// system_can_yield = 1;
				np_wakelock_helper();
				ret = pm_changestate(PM_IDLE_DOMAIN, PM_NORMAL);
				if (ret < 0) {
					oldstate = PM_NORMAL;
				}
				printf("Wakeup from Sleep!!\n");

				break;
			default:
				break;
		}
			//TODO: Handle critical section access logic for SMP case in 8730E?
			//Is leave_critical_section required? In accordance with irqsave()^
	}
EXIT2:
	if(oldstate == PM_STANDBY && newstate != PM_SLEEP) {
		np_wakelock_helper();
	}
}
#else
#define up_idlepm()
#endif

/****************************************************************************
 * Name: up_idle
 *
 * Description:
 *   up_idle() is the logic that will be executed when there is no other
 *   ready-to-run task.  This is processor idle time and will continue until
 *   some interrupt occurs to cause a context switch from the idle task.
 *
 *   Processing in this state may be processor-specific. e.g., this is where
 *   power management operations might be performed.
 *
 ****************************************************************************/

void up_idle(void)
{
#if defined(CONFIG_SUPPRESS_INTERRUPTS) || defined(CONFIG_SUPPRESS_TIMER_INTS)
	/* If the system is idle and there are no timer interrupts, then process
	 * "fake" timer interrupts. Hopefully, something will wake up.
	 */

	nxsched_process_timer();
#else

	/* Sleep until an interrupt occurs to save power,
	   is it possible to toggle between HW/SW sleep, to
	   lower down average power consumption?
	 */
	// asm("WFI");
	up_idlepm();
#endif
}

#ifdef CONFIG_PM
void arm_pminitialize(void)
{
	/* Then initialize the TinyAra power management subsystem proper */
	pm_initialize();
}
#endif