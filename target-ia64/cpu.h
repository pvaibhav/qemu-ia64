/*
 * IA64 virtual CPU header
 *
 *  Copyright (c) 2011 Prashant Vaibhav
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
#ifndef CPU_IA64_H
#define CPU_IA64_H

#define TARGET_LONG_BITS 64

#define ELF_MACHINE	EM_IA_64

#define CPUState struct CPUIA64State

#include "cpu-defs.h"
#define TARGET_PAGE_BITS 12

#define TARGET_PHYS_ADDR_SPACE_BITS 64
#define TARGET_VIRT_ADDR_SPACE_BITS 64

#include "cpu-all.h"

#include "softfloat.h"

#define NB_MMU_MODES 2 // guess
#define MMU_USER_IDX 0 // guess

typedef struct CPUIA64State {
    uint64_t    gr[128];    // general registers gr0 - gr127
    double      fr[128];    // floating point registers
    uint64_t    pr;         // predicate registers, pr[qp] = 1 & (pr >> qp)
#define IA64_BIT_IS_SET(p,x) (1 & ((p) >> (x)))
#define IA64_SET_BIT(p,x)    ((p) = (p) | (1 << (x)))
    
    uint64_t    br[8];      // branch registers br0 - br7
    uint64_t    ar[128];    // application registers ar0 - ar127
    uint64_t    ip;         // instruction pointer (PC)
    struct {                // current frame marker (has various fields)
        uint8_t sof;
        uint8_t sol;
        uint8_t sor;
        struct {
            uint8_t gr;
            uint8_t fr;
            uint8_t pr;
        } rrb;
    } cfm;

    // TODO: cpuid, pmd, user mask and alat registers

    CPU_COMMON
} CPUIA64State;

#if defined(CONFIG_USER_ONLY)
static inline void cpu_clone_regs(CPUState *env, target_ulong newsp)
{
    // TODO
}
#endif

CPUIA64State *cpu_ia64_init(const char *cpu_model);
int cpu_ia64_exec(CPUIA64State *s);
void cpu_ia64_close(CPUIA64State *s);

/* you can call this signal handler from your SIGBUS and SIGSEGV
   signal handlers to inform the virtual CPU of exceptions. non zero
   is returned if the signal was handled by the virtual CPU.  */
int cpu_ia64_signal_handler(int host_signum, void *pinfo,
                           void *puc);
int cpu_ia64_handle_mmu_fault (CPUIA64State *env, target_ulong address, int rw,
                              int mmu_idx, int is_softmuu);

#define cpu_handle_mmu_fault cpu_ia64_handle_mmu_fault
#define cpu_init cpu_ia64_init
#define cpu_exec cpu_ia64_exec
#define cpu_gen_code cpu_ia64_gen_code
#define cpu_signal_handler cpu_ia64_signal_handler

#include "exec-all.h"

enum {
    EXCP_SYSCALL_BREAK, /* syscall via break.m 0x10000  */
    EXCP_MMFAULT
};

static inline int cpu_has_work(CPUState *env)
{
    return env->interrupt_request & CPU_INTERRUPT_HARD; // guess
}

static inline void cpu_pc_from_tb(CPUState *env, TranslationBlock* tb)
{
    env->ip = tb->pc;
}

static inline void cpu_get_tb_cpu_state(CPUState* env, target_ulong *pc,
                                        target_ulong *cs_base, int *flags)
{
    *pc = env->ip;
    *cs_base = 0; // XXX: makes sense for itanium ?
    *flags = 0; // XXX
}
#endif
