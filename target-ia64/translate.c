/*
 *  IA64 translation
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

#include "cpu.h"
#include "exec-all.h"
#include "disas.h"
#include "tcg-op.h"
#include "qemu-log.h"

void cpu_dump_state(CPUState *env, FILE *f,
                    int (*cpu_fprintf)(FILE *f, const char *fmt, ...),
                    int flags)
{
    int i;
    for (i = 0; i < 128; i++) {
        cpu_fprintf(f, "gr%02d=%016lx", i, env->gr[i]);
        if ((i % 4) == 3) {
            cpu_fprintf(f, "\n");
        } else {
            cpu_fprintf(f, " ");
        }
    }
    // TODO: print other regs
}

void gen_intermediate_code (CPUState *env, struct TranslationBlock *tb)
{
    printf("gen_intermediate_code called, pc=%lu, size=%u, flags=0x%lx\n", tb->pc, tb->size, (unsigned long)tb->flags);
    printf("One byte there: %x\n", ldub_code(env->ip));
}

void gen_intermediate_code_pc (CPUState *env, struct TranslationBlock *tb)
{
    printf("gen_intermediate_code_pc called\n");
}

void restore_state_to_opc(CPUState *env, TranslationBlock *tb, int pc_pos)
{
    printf("restore_state_to_opc called\n");
}
