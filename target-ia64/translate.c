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
#include "helpers.h"
#define GEN_HELPER 1
#include "helpers.h"

/* TCG globals to map to cpu registers and other state */
static TCGv_ptr reg_env;        // CPUState struct pointer
static TCGv_i64 reg_gr[128];    // general registers
static TCGv_i64 reg_br[8];      // branch registers
static TCGv_i64 reg_ip;         // instruction pointer
static TCGv_i64 reg_ar_pfs;     // application register ar.pfs
static TCGv_i32 reg_sol;        // fields from cfm
static TCGv_i32 reg_sof;
static TCGv_i32 reg_sor;

static const char* reg_name_r[] = {
    "r0",
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

static const char* reg_name_b[] = {
    "b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7"
};

void cpu_ia64_tcg_init(void) {
    int r;
    
    reg_env = tcg_global_reg_new_ptr(TCG_AREG0, "env");
    
    reg_ip = tcg_global_mem_new_i64(TCG_AREG0, offsetof(CPUState, ip), "ip");

    for (r = 0; r < 128; r++) {
        reg_gr[r] = tcg_global_mem_new_i64(TCG_AREG0, offsetof(CPUState, gr[r]),
                                       reg_name_r[r]);
    }
    
    for (r = 0; r < 8; r++) {
        reg_br[r] = tcg_global_mem_new_i64(TCG_AREG0, offsetof(CPUState, br[r]),
                                           reg_name_b[r]);
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
                    int (*cpu_fqemu_log)(FILE *f, const char *fmt, ...),
                    int flags)
{
    int i;
    for (i = 0; i < 128; i++) {
        cpu_fqemu_log(f, "gr%02d=%016lx", i, env->gr[i]);
        if ((i % 4) == 3) {
            cpu_fqemu_log(f, "\n");
        } else {
            cpu_fqemu_log(f, " ");
        }
    }
    // TODO: print other regs
}

static inline void gen_save_ip(void* ip_in_host) {
    tcg_gen_movi_i64(reg_ip, h2g((uint64_t) ip_in_host));
}

static inline void gen_exception(unsigned int excp) {
    TCGv_i32 tmp = tcg_temp_new_i32();
    tcg_gen_movi_i32(tmp, excp);
    gen_helper_exception(tmp);
    tcg_temp_free_i32(tmp);
}

static inline void gen_op_alloc(const uint8_t gr, const uint8_t i,
                                const uint8_t l,  const uint8_t o,
                                const uint8_t r)
{
    /* alloc r[gr] = ar.pfs, i, l, o, r */
    if ((i + l + o) > 96 || r > (i + l)) {
        // TODO: raise illegal operation
    } else {
        tcg_gen_movi_i32(reg_sol, i + l);
        tcg_gen_movi_i32(reg_sof, i + l + o);
        tcg_gen_movi_i32(reg_sor, r);
        tcg_gen_mov_i64(reg_gr[gr], reg_ar_pfs); // XXX: check 'gr'
    }
}

static inline void gen_op_mov_rr(const uint8_t dest, const uint8_t source)
{
    /* mov r[dest] = r[source] */
    qemu_log("r%d = r%d", dest, source);
    if (source == 0) {
        tcg_gen_movi_i64(reg_gr[dest], 0);
    } else {
        tcg_gen_mov_i64(reg_gr[dest], reg_gr[source]);
    }
}

static inline void gen_op_mov_rb(const uint8_t dest, const uint8_t source)
{
    /* mov r[dest] = b[source] */
    qemu_log("r%d = b%d", dest, source);
    tcg_gen_mov_i64(reg_gr[dest], reg_br[source]);
}

static inline void gen_op_mov_r_ip(const uint8_t dest, void* ip)
{
    /* mov r[dest] = ip */
    qemu_log("r%d = ip", dest);
    gen_save_ip(ip);
    TCGv_i64 tmp = tcg_temp_new_i64();
    tcg_gen_mov_i64(tmp, reg_ip);
    tcg_gen_andi_i64(reg_gr[dest], tmp, ~0x03);
    tcg_temp_free_i64(tmp);
}

static inline void gen_op_add_rri(const uint8_t dest,
                                  const uint8_t src,
                                  const uint32_t imm)
{
    /* add r[dest] = r[src] + imm */
    qemu_log("r%d = r%d + %d", dest, src, imm);
    if (src == 0 /* r0 is always zero */) {
        tcg_gen_movi_i64(reg_gr[dest], (uint64_t) imm);
    } else {
        tcg_gen_addi_i64(reg_gr[dest], reg_gr[src], (uint64_t) imm);
    }
}

void gen_intermediate_code (CPUState *env, struct TranslationBlock *tb)
{
    qemu_log("gen_intermediate_code pc=0x%lx, size=%u, flags=0x%lx, end=0x%lx\n",
           tb->pc, tb->size, (unsigned long)tb->flags, env->code_ends_at);
    
    void* ip = g2h(env->ip); // get pointer to the entry point
    void* endpoint = g2h(env->code_ends_at);
    
    struct ia64_bundle bundle;
    struct ia64_inst* insn;
    int slot, slots_to_skip = 0;
    
    /* ip is actually ip + slot number */
    slots_to_skip = ((uint64_t) ip & 0x0f);
    ip = (void*) ((uint64_t) ip & ~0x0f);
    
    tb->size = 0;
    
    qemu_log("ip is 0x%lx, slots to skip=%d\n", (uint64_t) ip, slots_to_skip);
    
    for (;;) {
        /* decode one bundle */
        ia64_decode(ip, &bundle);
        
        if (bundle.b_templ == 0 /* invalid insn */) {
            break;
        }
        for (slot = 0; slot < 3; slot++) {
            if (slots_to_skip-- > 0) {
                qemu_log("Skipping slot %d\n", slot);
                continue;
            }
            
            insn = &bundle.b_inst[slot];
            
            // FIXME: predicate is ignored
            qemu_log("[%d]\t", slot);
            switch (insn->i_op) {
            case IA64_OP_ALLOC:
                qemu_log("alloc\n");
                gen_op_alloc(insn->i_oper[1].o_value, /* r# */
                             insn->i_oper[3].o_value, /* i */
                             insn->i_oper[4].o_value, /* l */
                             insn->i_oper[5].o_value, /* o */
                             insn->i_oper[6].o_value  /* r */);
                break;
            case IA64_OP_ADD:
            case IA64_OP_ADDS:
            case IA64_OP_ADDL:
            case IA64_OP_ADDP4:
                qemu_log("adds (%d)\t", insn->i_op);
                if (insn->i_oper[2].o_type == IA64_OPER_IMM &&
                    insn->i_oper[2].o_value == 0)
                {
                    /* this is an add r[dest] = r[src], 0, i.e. just a
                       mov r[dest] = r[src] */
                    gen_op_mov_rr(insn->i_oper[1].o_value,
                                  insn->i_oper[3].o_value);
                } else if (insn->i_oper[2].o_type == IA64_OPER_IMM &&
                           insn->i_oper[3].o_type == IA64_OPER_GREG) {
                    gen_op_add_rri(insn->i_oper[1].o_value,
                                   insn->i_oper[3].o_value,
                                   insn->i_oper[2].o_value);

                } else {
                    qemu_log("unhandled");
                }
                qemu_log("\n");
                break;
            case IA64_OP_NOP:
                qemu_log("nop\n");
                break;
            case IA64_OP_MOV:
            case IA64_OP_MOVL:
                qemu_log("mov%s\t", insn->i_op == IA64_OP_MOVL ? "l" : "");
                if (insn->i_oper[1].o_type != IA64_OPER_GREG) {
                    /* cannot handle non-gr destination */
                    qemu_log("non-gr destination (type %d)\n",
                           insn->i_oper[1].o_type);
                    break;
                }
                switch (insn->i_oper[2].o_type) {
                    case IA64_OPER_BREG:
                        gen_op_mov_rb(insn->i_oper[1].o_value,
                                      insn->i_oper[2].o_value);
                        break;
                    case IA64_OPER_GREG:
                        gen_op_mov_rr(insn->i_oper[1].o_value,
                                      insn->i_oper[2].o_value);
                        break;
                    case IA64_OPER_IP:
                        gen_op_mov_r_ip(insn->i_oper[1].o_value, ip);
                        break;
                    default:
                        qemu_log("Unhandled MOV (source oper type %d)\n",
                               insn->i_oper[2].o_type);
                };
                qemu_log("\n");
                break;
            case IA64_OP_BREAK:
                qemu_log("break\n");
                slot++;
                gen_save_ip(ip + slot);
                gen_exception(EXCP_SYSCALL_BREAK);
                goto done_with_tb;
                break;
            default:
                qemu_log("Unhandled opcode %d\n", insn->i_op);
                break;
            };
        } /* slot */
        ip += 16; // update PC (each bundle is 16 bytes in length)
        tb->size += 16;
        if (ip > endpoint) {
            qemu_log("\n\tCODE ENDS!\n");
            gen_save_ip(ip + slot);
            goto done_with_tb;
            break;
        } else {
            qemu_log("Current IP: 0x%lx\n", (unsigned long)ip);
        }
    } /* bundle */
    
done_with_tb:
    qemu_log("Exiting TB with size=0x%x, ip=0x%lx\n", tb->size,
             (unsigned long)ip+slot);
    tcg_gen_exit_tb(0);
    *gen_opc_ptr = INDEX_op_end;
}

void gen_intermediate_code_pc (CPUState *env, struct TranslationBlock *tb)
{
    qemu_log("gen_intermediate_code_pc called\n");
}

void restore_state_to_opc(CPUState *env, TranslationBlock *tb, int pc_pos)
{
    qemu_log("restore_state_to_opc called\n");
}
