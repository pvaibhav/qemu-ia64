/*
 *  IA64 helpers
 *
 *  Copyright (c) Prashant Vaibhav
 *  Copyright (c) 2009 Ulrich Hecht
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "exec-all.h"
#include "gdbstub.h"
#include "qemu-common.h"

CPUIA64State *cpu_ia64_init(const char *cpu_model)
{
    CPUIA64State *env;
    static int inited = 0;

    env = qemu_mallocz(sizeof(CPUIA64State));
    cpu_exec_init(env);
    if (!inited) {
        inited = 1;
    }
    
    cpu_ia64_tcg_init();
    
    env->cpu_model_str = cpu_model;
    cpu_reset(env);
    qemu_init_vcpu(env);
    return env;
}

void cpu_reset(CPUIA64State *env)
{
    if (qemu_loglevel_mask(CPU_LOG_RESET)) {
        qemu_log("CPU Reset (CPU %d)\n", env->cpu_index);
        log_cpu_state(env, 0);
    }

    memset(env, 0, offsetof(CPUIA64State, breakpoints));
    /* FIXME: reset vector? */
    tlb_flush(env, 1);
}

int cpu_ia64_handle_mmu_fault (CPUState *env, target_ulong address, int rw,
                              int mmu_idx, int is_softmmu)
{
    env->exception_index = EXCP_MMFAULT;
    return 1;
}
