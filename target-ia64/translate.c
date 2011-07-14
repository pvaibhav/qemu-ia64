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
#include "ia64_disas.h"

/* TCG globals to map to cpu registers and other state */
static TCGv_i64 reg_gr[128];    // general registers
static TCGv_i64 reg_ip;         // instruction pointer
static TCGv_i64 reg_ar_pfs;     // application register ar.pfs
static TCGv_i32 reg_sol;        // fields from cfm
static TCGv_i32 reg_sof;
static TCGv_i32 reg_sor;

static const char* reg_name_r[] = {
    "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
    "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16",
    "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24",
    "r25", "r26", "r27", "r28", "r29", "r30", "r31", "r32",
    "r33", "r34", "r35", "r36", "r37", "r38", "r39", "r40",
    "r41", "r42", "r43", "r44", "r45", "r46", "r47", "r48",
    "r49", "r50", "r51", "r52", "r53", "r54", "r55", "r56",
    "r57", "r58", "r59", "r60", "r61", "r62", "r63", "r64",
    "r65", "r66", "r67", "r68", "r69", "r70", "r71", "r72",
    "r73", "r74", "r75", "r76", "r77", "r78", "r79", "r80",
    "r81", "r82", "r83", "r84", "r85", "r86", "r87", "r88",
    "r89", "r90", "r91", "r92", "r93", "r94", "r95", "r96",
    "r97", "r98", "r99", "r100", "r101", "r102", "r103", "r104",
    "r105", "r106", "r107", "r108", "r109", "r110", "r111", "r112",
    "r113", "r114", "r115", "r116", "r117", "r118", "r119", "r120",
    "r121", "r122", "r123", "r124", "r125", "r126", "r127"
};

void cpu_ia64_tcg_init(void) {
    int r;
    
    reg_ip = tcg_global_mem_new_i64(TCG_AREG0, offsetof(CPUState, ip), "ip");

    reg_gr[0] = tcg_const_i64(0); /* XXX: need to make a global? */
    for (r = 1; r < 128; r++) {
        reg_gr[r] = tcg_global_mem_new_i64(TCG_AREG0, offsetof(CPUState, gr[r]),
                                       reg_name_r[r]);
    }
    
    reg_sol = tcg_global_mem_new_i32(TCG_AREG0, offsetof(CPUState, cfm.sol),
                                 "cfm.sol");
    reg_sof = tcg_global_mem_new_i32(TCG_AREG0, offsetof(CPUState, cfm.sof),
                                 "cfm.sof");
    reg_sor = tcg_global_mem_new_i32(TCG_AREG0, offsetof(CPUState, cfm.sor),
                                 "cfm.sor");

    reg_ar_pfs = tcg_global_mem_new_i64(TCG_AREG0,
                                        offsetof(CPUState, ar[ar_pfs]),
                                        "ar.pfs");
}


/****************************************************************************
 *                            Translation Code                              *
 ****************************************************************************/

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

static void gen_op_alloc(const uint8_t gr, const uint8_t i, const uint8_t l,
                  const uint8_t o, const uint8_t r)
{
    /* alloc r[gr] = ar.pfs, i, l, o, r */
    if ((i + l + o) > 96 || r > (i + l)) {
        // TODO: raise illegal operation
    } else {
        tcg_gen_movi_tl(reg_sol, i + l);
        tcg_gen_movi_tl(reg_sof, i + l + o);
        tcg_gen_movi_tl(reg_sor, r);
        tcg_gen_mov_tl(reg_gr[gr], reg_ar_pfs); // XXX: check 'gr'
    }
}
#if 0
static void gen_op_mov_rr(const uint8_t dest, const uint8_t source)
{
    /* mov r[dest] = r[source] */
    tcg_gen_mov_tl(reg_gr[dest], reg_gr[source]);
}
#endif
void gen_intermediate_code (CPUState *env, struct TranslationBlock *tb)
{
    printf("gen_intermediate_code called, pc=%lu, size=%u, flags=0x%lx\n", tb->pc, tb->size, (unsigned long)tb->flags);
    
    void* ip = g2h(env->ip); // get pointer to the entry point
    
    struct ia64_bundle bundle;
    struct ia64_inst* insn;
    int slot;
    
    for (;;) {
        ia64_decode(ip, &bundle);
        if (bundle.b_templ == 0 /* invalid insn */) {
            break;
        }
        ip += 16; // update PC (each bundle is 16 bytes in length)
        for (slot = 0; slot < 3; slot++) {
            insn = &bundle.b_inst[slot];
            // FIXME: predicate is ignored
            switch (insn->i_op) {
                case IA64_OP_ALLOC:
                    gen_op_alloc(insn->i_oper[1].o_value, /* r# */
                                 insn->i_oper[3].o_value, /* i */
                                 insn->i_oper[4].o_value, /* l */
                                 insn->i_oper[5].o_value, /* o */
                                 insn->i_oper[6].o_value  /* r */);
                    break;
                case IA64_OP_MOV:
                case IA64_OP_MOVL:
                    printf("MOV opcode %d\n", insn->i_op);
                    break;
                default:
                    printf("\nUnhandled opcode %d in slot [%d]",
                           insn->i_op, slot);
                    break;
            };
        }
    }
    env->ip = h2g(ip);
}

void gen_intermediate_code_pc (CPUState *env, struct TranslationBlock *tb)
{
    printf("gen_intermediate_code_pc called\n");
}

void restore_state_to_opc(CPUState *env, TranslationBlock *tb, int pc_pos)
{
    printf("restore_state_to_opc called\n");
}
