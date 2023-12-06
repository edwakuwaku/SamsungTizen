/****************************************************************************
 *
 * Copyright 2023 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * pm/pm_ioctl.c
 *
 *   Copyright (C) 2011-2012 Gregory Nutt. All rights reserved.
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
 *  *    used to endorse or promote products derived from this software
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
#include <assert.h>
#include <tinyara/pm/pm.h>
#include <tinyara/clock.h>
#include <tinyara/irq.h>

#include "pm.h"

#ifdef CONFIG_PM

/****************************************************************************
 * Public Functions
 ****************************************************************************/
struct timer_s g_timer_wakeup = {0, 0};
/****************************************************************************
 * Name: pm_ioctl
 *
 * Description:
 *   This function can be called from applications running on TizenRT to
 *   request/enable/disable power state transitions. This function conveys
 *   the request from the application to the PM module, which thereafter
 *   checks the feasibility of the requests. There is no returned value.
 *
 *   The power management state is not automatically changed, however.
 *   up_idlepm() must be called in order to make the state change.
 *
 *   These two steps are separated because the application is not supposed
 *   to invoke power state changes directly, it can only request/suggest.
 *
 * Input Parameters:
 *   domain - the PM domain to check
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/
void pm_ioctl(enum pm_ioctl_cmds cmd, uint32_t TimerInterval)
{
    FAR struct pm_domain_s *pdom = &g_pmglobals.domain[PM_IDLE_DOMAIN];
	irqstate_t flags;
	flags = irqsave();
	switch(cmd) {
		case PM_IOC_NOCMD:
			pmdbg("Nothing to do\n");
			break;
		case PM_IOC_STAY:
			//Lock the current state due to some ongoing activity
			(void) TimerInterval;
			pm_stay(PM_IDLE_DOMAIN, pm_querystate(PM_IDLE_DOMAIN));
			break;
		case PM_IOC_RELAX:
			//Relax the current state due to low activity usage
			pm_relax(PM_IDLE_DOMAIN, pm_querystate(PM_IDLE_DOMAIN));
			break;
		/* Note that in the above 2 IOCTL calls, the application does not provide the state to stay/relax.
		 * The application cannot anyways know the pm_domain of the current thread.
		 * The thread will be extracted during the processing of the IOCTL call itself.
		 * The state to stay in is the current state itself, and the state to be relaxed from will also
		 * be the current state itself. This is based on the assumption that if an application is requesting
		 * a stay, then the current state is capable enough for the application to do its work, hence we will
		 * stay in the current request only. If a stay was invoked in the current PM state, then it should be
		 * relaxed from the same state too.
		 * Open to discussion and consideration
		 */
		case PM_IOC_SLEEP:
			//Preset timer countdown value, and update next recommended state to sleep
            if (TimerInterval > 0) {
                g_timer_wakeup.use_timer = 1;
                g_timer_wakeup.timer_interval = TimerInterval;
            }

            pdom->recommended = PM_SLEEP;
			/* Invoke up_idlepm() from app thread, but the design of the loop structure is yet to be revised? */
			// up_idlepm();
			break;

			// Might have to consider for dual core scenario in the future, leave it as it is first
			/* If another application running parallely on another core also requests sleep, then the IOCTL
                         * request will be processed for that particular core.
                         * If an application requests sleep and then gets switched out, either the sleep request is
                         * provided and the board is put to sleep, if it cannot be provided then the request will be
                         * discarded. This means that there should not be multiple sleep requests pending in line.
                         */
		default:
			pmdbg("Invalid PM IOCTLL command\n");
			break;
	}
	irqrestore(flags);
}
#endif