/*
 *  IA64 helper routines
 *
 *  Copyright (c) 2009 Alexander Graf
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


/*-
 * Copyright (c) 2000-2006 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>

#include "ia64_disas_int.h"
#include "ia64_disas.h"

#define FRAG(o,l)	((int)((o << 8) | (l & 0xff)))
#define FRAG_OFS(f)	(f >> 8)
#define FRAG_LEN(f)	(f & 0xff)

/*
 * Support functions.
 */
static void
ia64_cmpltr_add(struct ia64_inst *i, enum ia64_cmpltr_class c,
                enum ia64_cmpltr_type t)
{
    
	i->i_cmpltr[i->i_ncmpltrs].c_class = c;
	i->i_cmpltr[i->i_ncmpltrs].c_type = t;
	i->i_ncmpltrs++;
	assert(i->i_ncmpltrs < 6);
}

static void
ia64_hint(struct ia64_inst *i, enum ia64_cmpltr_class c)
{
    
	switch (FIELD(i->i_bits, 28, 2)) { /* hint */
        case 0:
            ia64_cmpltr_add(i, c, IA64_CT_NONE);
            break;
        case 1:
            ia64_cmpltr_add(i, c, IA64_CT_NT1);
            break;
        case 2:
            ia64_cmpltr_add(i, c, IA64_CT_NT2);
            break;
        case 3:
            ia64_cmpltr_add(i, c, IA64_CT_NTA);
            break;
	}
}

static void
ia64_sf(struct ia64_inst *i)
{
    
	switch (FIELD(i->i_bits, 34, 2)) {
        case 0:
            ia64_cmpltr_add(i, IA64_CC_SF, IA64_CT_S0);
            break;
        case 1:
            ia64_cmpltr_add(i, IA64_CC_SF, IA64_CT_S1);
            break;
        case 2:
            ia64_cmpltr_add(i, IA64_CC_SF, IA64_CT_S2);
            break;
        case 3:
            ia64_cmpltr_add(i, IA64_CC_SF, IA64_CT_S3);
            break;
	}
}

static void
ia64_brhint(struct ia64_inst *i)
{
	uint64_t bits = i->i_bits;
    
	switch (FIELD(bits, 33, 2)) { /* bwh */
        case 0:
            ia64_cmpltr_add(i, IA64_CC_BWH, IA64_CT_SPTK);
            break;
        case 1:
            ia64_cmpltr_add(i, IA64_CC_BWH, IA64_CT_SPNT);
            break;
        case 2:
            ia64_cmpltr_add(i, IA64_CC_BWH, IA64_CT_DPTK);
            break;
        case 3:
            ia64_cmpltr_add(i, IA64_CC_BWH, IA64_CT_DPNT);
            break;
	}
    
	if (FIELD(bits, 12, 1)) /* ph */
		ia64_cmpltr_add(i, IA64_CC_PH, IA64_CT_MANY);
	else
		ia64_cmpltr_add(i, IA64_CC_PH, IA64_CT_FEW);
    
	if (FIELD(bits, 35, 1)) /* dh */
		ia64_cmpltr_add(i, IA64_CC_DH, IA64_CT_CLR);
	else
		ia64_cmpltr_add(i, IA64_CC_DH, IA64_CT_NONE);
}

static void
ia64_brphint(struct ia64_inst *i)
{
	uint64_t bits = i->i_bits;
    
	switch (FIELD(bits, 3, 2)) { /* ipwh, indwh */
        case 0:
            ia64_cmpltr_add(i, IA64_CC_IPWH, IA64_CT_SPTK);
            break;
        case 1:
            ia64_cmpltr_add(i, IA64_CC_IPWH, IA64_CT_LOOP);
            break;
        case 2:
            ia64_cmpltr_add(i, IA64_CC_IPWH, IA64_CT_DPTK);
            break;
        case 3:
            ia64_cmpltr_add(i, IA64_CC_IPWH, IA64_CT_EXIT);
            break;
	}
    
	if (FIELD(bits, 5, 1)) /* ph */
		ia64_cmpltr_add(i, IA64_CC_PH, IA64_CT_MANY);
	else
		ia64_cmpltr_add(i, IA64_CC_PH, IA64_CT_FEW);
    
	switch (FIELD(bits, 0, 3)) { /* pvec */
        case 0:
            ia64_cmpltr_add(i, IA64_CC_PVEC, IA64_CT_DC_DC);
            break;
        case 1:
            ia64_cmpltr_add(i, IA64_CC_PVEC, IA64_CT_DC_NT);
            break;
        case 2:
            ia64_cmpltr_add(i, IA64_CC_PVEC, IA64_CT_TK_DC);
            break;
        case 3:
            ia64_cmpltr_add(i, IA64_CC_PVEC, IA64_CT_TK_TK);
            break;
        case 4:
            ia64_cmpltr_add(i, IA64_CC_PVEC, IA64_CT_TK_NT);
            break;
        case 5:
            ia64_cmpltr_add(i, IA64_CC_PVEC, IA64_CT_NT_DC);
            break;
        case 6:
            ia64_cmpltr_add(i, IA64_CC_PVEC, IA64_CT_NT_TK);
            break;
        case 7:
            ia64_cmpltr_add(i, IA64_CC_PVEC, IA64_CT_NT_NT);
            break;
	}
    
	if (FIELD(bits, 35, 1)) /* ih */
		ia64_cmpltr_add(i, IA64_CC_IH, IA64_CT_IMP);
	else
		ia64_cmpltr_add(i, IA64_CC_IH, IA64_CT_NONE);
}

static enum ia64_oper_type
ia64_normalize(struct ia64_inst *i, enum ia64_op op)
{
	enum ia64_oper_type ot = IA64_OPER_NONE;
    
	switch (op) {
        case IA64_OP_BR_CALL:
            ia64_cmpltr_add(i, IA64_CC_BTYPE, IA64_CT_CALL);
            op = IA64_OP_BR;
            break;
        case IA64_OP_BR_CEXIT:
            ia64_cmpltr_add(i, IA64_CC_BTYPE, IA64_CT_CEXIT);
            op = IA64_OP_BR;
            break;
        case IA64_OP_BR_CLOOP:
            ia64_cmpltr_add(i, IA64_CC_BTYPE, IA64_CT_CLOOP);
            op = IA64_OP_BR;
            break;
        case IA64_OP_BR_COND:
            ia64_cmpltr_add(i, IA64_CC_BTYPE, IA64_CT_COND);
            op = IA64_OP_BR;
            break;
        case IA64_OP_BR_CTOP:
            ia64_cmpltr_add(i, IA64_CC_BTYPE, IA64_CT_CTOP);
            op = IA64_OP_BR;
            break;
        case IA64_OP_BR_IA:
            ia64_cmpltr_add(i, IA64_CC_BTYPE, IA64_CT_IA);
            op = IA64_OP_BR;
            break;
        case IA64_OP_BR_RET:
            ia64_cmpltr_add(i, IA64_CC_BTYPE, IA64_CT_RET);
            op = IA64_OP_BR;
            break;
        case IA64_OP_BR_WEXIT:
            ia64_cmpltr_add(i, IA64_CC_BTYPE, IA64_CT_WEXIT);
            op = IA64_OP_BR;
            break;
        case IA64_OP_BR_WTOP:
            ia64_cmpltr_add(i, IA64_CC_BTYPE, IA64_CT_WTOP);
            op = IA64_OP_BR;
            break;
        case IA64_OP_BREAK_B:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_B);
            op = IA64_OP_BREAK;
            break;
        case IA64_OP_BREAK_F:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_F);
            op = IA64_OP_BREAK;
            break;
        case IA64_OP_BREAK_I:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_I);
            op = IA64_OP_BREAK;
            break;
        case IA64_OP_BREAK_M:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_M);
            op = IA64_OP_BREAK;
            break;
        case IA64_OP_BREAK_X:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_X);
            op = IA64_OP_BREAK;
            break;
        case IA64_OP_BRL_COND:
            ia64_cmpltr_add(i, IA64_CC_BTYPE, IA64_CT_COND);
            op = IA64_OP_BRL;
            break;
        case IA64_OP_BRL_CALL:
            ia64_cmpltr_add(i, IA64_CC_BTYPE, IA64_CT_CALL);
            op = IA64_OP_BRL;
            break;
        case IA64_OP_BRP_:
            ia64_cmpltr_add(i, IA64_CC_BTYPE, IA64_CT_NONE);
            op = IA64_OP_BRP;
            break;
        case IA64_OP_BRP_RET:
            ia64_cmpltr_add(i, IA64_CC_BTYPE, IA64_CT_RET);
            op = IA64_OP_BRP;
            break;
        case IA64_OP_BSW_0:
            ia64_cmpltr_add(i, IA64_CC_BSW, IA64_CT_0);
            op = IA64_OP_BSW;
            break;
        case IA64_OP_BSW_1:
            ia64_cmpltr_add(i, IA64_CC_BSW, IA64_CT_1);
            op = IA64_OP_BSW;
            break;
        case IA64_OP_CHK_A_CLR:
            ia64_cmpltr_add(i, IA64_CC_CHK, IA64_CT_A);
            ia64_cmpltr_add(i, IA64_CC_ACLR, IA64_CT_CLR);
            op = IA64_OP_CHK;
            break;
        case IA64_OP_CHK_A_NC:
            ia64_cmpltr_add(i, IA64_CC_CHK, IA64_CT_A);
            ia64_cmpltr_add(i, IA64_CC_ACLR, IA64_CT_NC);
            op = IA64_OP_CHK;
            break;
        case IA64_OP_CHK_S:
            ia64_cmpltr_add(i, IA64_CC_CHK, IA64_CT_S);
            op = IA64_OP_CHK;
            break;
        case IA64_OP_CHK_S_I:
            ia64_cmpltr_add(i, IA64_CC_CHK, IA64_CT_S);
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_I);
            op = IA64_OP_CHK;
            break;
        case IA64_OP_CHK_S_M:
            ia64_cmpltr_add(i, IA64_CC_CHK, IA64_CT_S);
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_M);
            op = IA64_OP_CHK;
            break;
        case IA64_OP_CLRRRB_:
            ia64_cmpltr_add(i, IA64_CC_CLRRRB, IA64_CT_NONE);
            op = IA64_OP_CLRRRB;
            break;
        case IA64_OP_CLRRRB_PR:
            ia64_cmpltr_add(i, IA64_CC_CLRRRB, IA64_CT_PR);
            op = IA64_OP_CLRRRB;
            break;
        case IA64_OP_CMP_EQ:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_EQ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_NONE);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_EQ_AND:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_EQ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_EQ_OR:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_EQ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_EQ_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_EQ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_EQ_UNC:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_EQ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_UNC);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_GE_AND:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_GE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_GE_OR:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_GE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_GE_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_GE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_GT_AND:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_GT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_GT_OR:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_GT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_GT_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_GT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_LE_AND:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_LE_OR:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_LE_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_LT:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_NONE);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_LT_AND:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_LT_OR:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_LT_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_LT_UNC:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_UNC);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_LTU:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LTU);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_NONE);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_LTU_UNC:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LTU);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_UNC);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_NE_AND:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_NE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_NE_OR:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_NE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP_NE_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_NE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_CMP;
            break;
        case IA64_OP_CMP4_EQ:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_EQ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_NONE);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_EQ_AND:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_EQ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_EQ_OR:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_EQ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_EQ_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_EQ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_EQ_UNC:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_EQ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_UNC);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_GE_AND:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_GE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_GE_OR:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_GE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_GE_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_GE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_GT_AND:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_GT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_GT_OR:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_GT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_GT_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_GT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_LE_AND:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_LE_OR:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_LE_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_LT:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_NONE);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_LT_AND:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_LT_OR:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_LT_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_LT_UNC:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LT);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_UNC);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_LTU:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LTU);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_NONE);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_LTU_UNC:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_LTU);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_UNC);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_NE_AND:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_NE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_NE_OR:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_NE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP4_NE_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_CREL, IA64_CT_NE);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_CMP4;
            break;
        case IA64_OP_CMP8XCHG16_ACQ:
            ia64_cmpltr_add(i, IA64_CC_SEM, IA64_CT_ACQ);
            op = IA64_OP_CMP8XCHG16;
            break;
        case IA64_OP_CMP8XCHG16_REL:
            ia64_cmpltr_add(i, IA64_CC_SEM, IA64_CT_REL);
            op = IA64_OP_CMP8XCHG16;
            break;
        case IA64_OP_CMPXCHG1_ACQ:
            ia64_cmpltr_add(i, IA64_CC_SEM, IA64_CT_ACQ);
            op = IA64_OP_CMPXCHG1;
            break;
        case IA64_OP_CMPXCHG1_REL:
            ia64_cmpltr_add(i, IA64_CC_SEM, IA64_CT_REL);
            op = IA64_OP_CMPXCHG1;
            break;
        case IA64_OP_CMPXCHG2_ACQ:
            ia64_cmpltr_add(i, IA64_CC_SEM, IA64_CT_ACQ);
            op = IA64_OP_CMPXCHG2;
            break;
        case IA64_OP_CMPXCHG2_REL:
            ia64_cmpltr_add(i, IA64_CC_SEM, IA64_CT_REL);
            op = IA64_OP_CMPXCHG2;
            break;
        case IA64_OP_CMPXCHG4_ACQ:
            ia64_cmpltr_add(i, IA64_CC_SEM, IA64_CT_ACQ);
            op = IA64_OP_CMPXCHG4;
            break;
        case IA64_OP_CMPXCHG4_REL:
            ia64_cmpltr_add(i, IA64_CC_SEM, IA64_CT_REL);
            op = IA64_OP_CMPXCHG4;
            break;
        case IA64_OP_CMPXCHG8_ACQ:
            ia64_cmpltr_add(i, IA64_CC_SEM, IA64_CT_ACQ);
            op = IA64_OP_CMPXCHG8;
            break;
        case IA64_OP_CMPXCHG8_REL:
            ia64_cmpltr_add(i, IA64_CC_SEM, IA64_CT_REL);
            op = IA64_OP_CMPXCHG8;
            break;
        case IA64_OP_CZX1_L:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_L);
            op = IA64_OP_CZX1;
            break;
        case IA64_OP_CZX1_R:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_R);
            op = IA64_OP_CZX1;
            break;
        case IA64_OP_CZX2_L:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_L);
            op = IA64_OP_CZX2;
            break;
        case IA64_OP_CZX2_R:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_R);
            op = IA64_OP_CZX2;
            break;
        case IA64_OP_DEP_:
            ia64_cmpltr_add(i, IA64_CC_DEP, IA64_CT_NONE);
            op = IA64_OP_DEP;
            break;
        case IA64_OP_DEP_Z:
            ia64_cmpltr_add(i, IA64_CC_DEP, IA64_CT_Z);
            op = IA64_OP_DEP;
            break;
        case IA64_OP_FC_:
            ia64_cmpltr_add(i, IA64_CC_FC, IA64_CT_NONE);
            op = IA64_OP_FC;
            break;
        case IA64_OP_FC_I:
            ia64_cmpltr_add(i, IA64_CC_FC, IA64_CT_I);
            op = IA64_OP_FC;
            break;
        case IA64_OP_FCLASS_M:
            ia64_cmpltr_add(i, IA64_CC_FCREL, IA64_CT_M);
            op = IA64_OP_FCLASS;
            break;
        case IA64_OP_FCVT_FX:
            ia64_cmpltr_add(i, IA64_CC_FCVT, IA64_CT_FX);
            ia64_cmpltr_add(i, IA64_CC_TRUNC, IA64_CT_NONE);
            op = IA64_OP_FCVT;
            break;
        case IA64_OP_FCVT_FX_TRUNC:
            ia64_cmpltr_add(i, IA64_CC_FCVT, IA64_CT_FX);
            ia64_cmpltr_add(i, IA64_CC_TRUNC, IA64_CT_TRUNC);
            op = IA64_OP_FCVT;
            break;
        case IA64_OP_FCVT_FXU:
            ia64_cmpltr_add(i, IA64_CC_FCVT, IA64_CT_FXU);
            ia64_cmpltr_add(i, IA64_CC_TRUNC, IA64_CT_NONE);
            op = IA64_OP_FCVT;
            break;
        case IA64_OP_FCVT_FXU_TRUNC:
            ia64_cmpltr_add(i, IA64_CC_FCVT, IA64_CT_FXU);
            ia64_cmpltr_add(i, IA64_CC_TRUNC, IA64_CT_TRUNC);
            op = IA64_OP_FCVT;
            break;
        case IA64_OP_FCVT_XF:
            ia64_cmpltr_add(i, IA64_CC_FCVT, IA64_CT_XF);
            ia64_cmpltr_add(i, IA64_CC_TRUNC, IA64_CT_NONE);
            op = IA64_OP_FCVT;
            break;
        case IA64_OP_FETCHADD4_ACQ:
            ia64_cmpltr_add(i, IA64_CC_SEM, IA64_CT_ACQ);
            op = IA64_OP_FETCHADD4;
            break;
        case IA64_OP_FETCHADD4_REL:
            ia64_cmpltr_add(i, IA64_CC_SEM, IA64_CT_REL);
            op = IA64_OP_FETCHADD4;
            break;
        case IA64_OP_FETCHADD8_ACQ:
            ia64_cmpltr_add(i, IA64_CC_SEM, IA64_CT_ACQ);
            op = IA64_OP_FETCHADD8;
            break;
        case IA64_OP_FETCHADD8_REL:
            ia64_cmpltr_add(i, IA64_CC_SEM, IA64_CT_REL);
            op = IA64_OP_FETCHADD8;
            break;
        case IA64_OP_FMA_:
            ia64_cmpltr_add(i, IA64_CC_PC, IA64_CT_NONE);
            op = IA64_OP_FMA;
            break;
        case IA64_OP_FMA_D:
            ia64_cmpltr_add(i, IA64_CC_PC, IA64_CT_D);
            op = IA64_OP_FMA;
            break;
        case IA64_OP_FMA_S:
            ia64_cmpltr_add(i, IA64_CC_PC, IA64_CT_S);
            op = IA64_OP_FMA;
            break;
        case IA64_OP_FMERGE_NS:
            ia64_cmpltr_add(i, IA64_CC_FMERGE, IA64_CT_NS);
            op = IA64_OP_FMERGE;
            break;
        case IA64_OP_FMERGE_S:
            ia64_cmpltr_add(i, IA64_CC_FMERGE, IA64_CT_S);
            op = IA64_OP_FMERGE;
            break;
        case IA64_OP_FMERGE_SE:
            ia64_cmpltr_add(i, IA64_CC_FMERGE, IA64_CT_SE);
            op = IA64_OP_FMERGE;
            break;
        case IA64_OP_FMIX_L:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_L);
            op = IA64_OP_FMIX;
            break;
        case IA64_OP_FMIX_LR:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_LR);
            op = IA64_OP_FMIX;
            break;
        case IA64_OP_FMIX_R:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_R);
            op = IA64_OP_FMIX;
            break;
        case IA64_OP_FMS_:
            ia64_cmpltr_add(i, IA64_CC_PC, IA64_CT_NONE);
            op = IA64_OP_FMS;
            break;
        case IA64_OP_FMS_D:
            ia64_cmpltr_add(i, IA64_CC_PC, IA64_CT_D);
            op = IA64_OP_FMS;
            break;
        case IA64_OP_FMS_S:
            ia64_cmpltr_add(i, IA64_CC_PC, IA64_CT_S);
            op = IA64_OP_FMS;
            break;
        case IA64_OP_FNMA_:
            ia64_cmpltr_add(i, IA64_CC_PC, IA64_CT_NONE);
            op = IA64_OP_FNMA;
            break;
        case IA64_OP_FNMA_D:
            ia64_cmpltr_add(i, IA64_CC_PC, IA64_CT_D);
            op = IA64_OP_FNMA;
            break;
        case IA64_OP_FNMA_S:
            ia64_cmpltr_add(i, IA64_CC_PC, IA64_CT_S);
            op = IA64_OP_FNMA;
            break;
        case IA64_OP_FPCMP_EQ:
            ia64_cmpltr_add(i, IA64_CC_FREL, IA64_CT_EQ);
            op = IA64_OP_FPCMP;
            break;
        case IA64_OP_FPCMP_LE:
            ia64_cmpltr_add(i, IA64_CC_FREL, IA64_CT_LE);
            op = IA64_OP_FPCMP;
            break;
        case IA64_OP_FPCMP_LT:
            ia64_cmpltr_add(i, IA64_CC_FREL, IA64_CT_LT);
            op = IA64_OP_FPCMP;
            break;
        case IA64_OP_FPCMP_NEQ:
            ia64_cmpltr_add(i, IA64_CC_FREL, IA64_CT_NEQ);
            op = IA64_OP_FPCMP;
            break;
        case IA64_OP_FPCMP_NLE:
            ia64_cmpltr_add(i, IA64_CC_FREL, IA64_CT_NLE);
            op = IA64_OP_FPCMP;
            break;
        case IA64_OP_FPCMP_NLT:
            ia64_cmpltr_add(i, IA64_CC_FREL, IA64_CT_NLT);
            op = IA64_OP_FPCMP;
            break;
        case IA64_OP_FPCMP_ORD:
            ia64_cmpltr_add(i, IA64_CC_FREL, IA64_CT_ORD);
            op = IA64_OP_FPCMP;
            break;
        case IA64_OP_FPCMP_UNORD:
            ia64_cmpltr_add(i, IA64_CC_FREL, IA64_CT_UNORD);
            op = IA64_OP_FPCMP;
            break;
        case IA64_OP_FPCVT_FX:
            ia64_cmpltr_add(i, IA64_CC_FCVT, IA64_CT_FX);
            ia64_cmpltr_add(i, IA64_CC_TRUNC, IA64_CT_NONE);
            op = IA64_OP_FPCVT;
            break;
        case IA64_OP_FPCVT_FX_TRUNC:
            ia64_cmpltr_add(i, IA64_CC_FCVT, IA64_CT_FX);
            ia64_cmpltr_add(i, IA64_CC_TRUNC, IA64_CT_TRUNC);
            op = IA64_OP_FPCVT;
            break;
        case IA64_OP_FPCVT_FXU:
            ia64_cmpltr_add(i, IA64_CC_FCVT, IA64_CT_FXU);
            ia64_cmpltr_add(i, IA64_CC_TRUNC, IA64_CT_NONE);
            op = IA64_OP_FPCVT;
            break;
        case IA64_OP_FPCVT_FXU_TRUNC:
            ia64_cmpltr_add(i, IA64_CC_FCVT, IA64_CT_FXU);
            ia64_cmpltr_add(i, IA64_CC_TRUNC, IA64_CT_TRUNC);
            op = IA64_OP_FPCVT;
            break;
        case IA64_OP_FPMERGE_NS:
            ia64_cmpltr_add(i, IA64_CC_FMERGE, IA64_CT_NS);
            op = IA64_OP_FPMERGE;
            break;
        case IA64_OP_FPMERGE_S:
            ia64_cmpltr_add(i, IA64_CC_FMERGE, IA64_CT_S);
            op = IA64_OP_FPMERGE;
            break;
        case IA64_OP_FPMERGE_SE:
            ia64_cmpltr_add(i, IA64_CC_FMERGE, IA64_CT_SE);
            op = IA64_OP_FPMERGE;
            break;
        case IA64_OP_FSWAP_:
            ia64_cmpltr_add(i, IA64_CC_FSWAP, IA64_CT_NONE);
            op = IA64_OP_FSWAP;
            break;
        case IA64_OP_FSWAP_NL:
            ia64_cmpltr_add(i, IA64_CC_FSWAP, IA64_CT_NL);
            op = IA64_OP_FSWAP;
            break;
        case IA64_OP_FSWAP_NR:
            ia64_cmpltr_add(i, IA64_CC_FSWAP, IA64_CT_NR);
            op = IA64_OP_FSWAP;
            break;
        case IA64_OP_FSXT_L:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_L);
            op = IA64_OP_FSXT;
            break;
        case IA64_OP_FSXT_R:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_R);
            op = IA64_OP_FSXT;
            break;
        case IA64_OP_GETF_D:
            ia64_cmpltr_add(i, IA64_CC_GETF, IA64_CT_D);
            op = IA64_OP_GETF;
            break;
        case IA64_OP_GETF_EXP:
            ia64_cmpltr_add(i, IA64_CC_GETF, IA64_CT_EXP);
            op = IA64_OP_GETF;
            break;
        case IA64_OP_GETF_S:
            ia64_cmpltr_add(i, IA64_CC_GETF, IA64_CT_S);
            op = IA64_OP_GETF;
            break;
        case IA64_OP_GETF_SIG:
            ia64_cmpltr_add(i, IA64_CC_GETF, IA64_CT_SIG);
            op = IA64_OP_GETF;
            break;
        case IA64_OP_HINT_B:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_B);
            op = IA64_OP_HINT;
            break;
        case IA64_OP_HINT_F:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_F);
            op = IA64_OP_HINT;
            break;
        case IA64_OP_HINT_I:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_I);
            op = IA64_OP_HINT;
            break;
        case IA64_OP_HINT_M:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_M);
            op = IA64_OP_HINT;
            break;
        case IA64_OP_HINT_X:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_X);
            op = IA64_OP_HINT;
            break;
        case IA64_OP_INVALA_:
            ia64_cmpltr_add(i, IA64_CC_INVALA, IA64_CT_NONE);
            op = IA64_OP_INVALA;
            break;
        case IA64_OP_INVALA_E:
            ia64_cmpltr_add(i, IA64_CC_INVALA, IA64_CT_E);
            op = IA64_OP_INVALA;
            break;
        case IA64_OP_ITC_D:
            ia64_cmpltr_add(i, IA64_CC_ITC, IA64_CT_D);
            op = IA64_OP_ITC;
            break;
        case IA64_OP_ITC_I:
            ia64_cmpltr_add(i, IA64_CC_ITC, IA64_CT_I);
            op = IA64_OP_ITC;
            break;
        case IA64_OP_ITR_D:
            ia64_cmpltr_add(i, IA64_CC_ITR, IA64_CT_D);
            ot = IA64_OPER_DTR;
            op = IA64_OP_ITR;
            break;
        case IA64_OP_ITR_I:
            ia64_cmpltr_add(i, IA64_CC_ITR, IA64_CT_I);
            ot = IA64_OPER_ITR;
            op = IA64_OP_ITR;
            break;
        case IA64_OP_LD1_:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_NONE);
            op = IA64_OP_LD1;
            break;
        case IA64_OP_LD1_A:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_A);
            op = IA64_OP_LD1;
            break;
        case IA64_OP_LD1_ACQ:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_ACQ);
            op = IA64_OP_LD1;
            break;
        case IA64_OP_LD1_BIAS:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_BIAS);
            op = IA64_OP_LD1;
            break;
        case IA64_OP_LD1_C_CLR:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_C_CLR);
            op = IA64_OP_LD1;
            break;
        case IA64_OP_LD1_C_CLR_ACQ:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_C_CLR_ACQ);
            op = IA64_OP_LD1;
            break;
        case IA64_OP_LD1_C_NC:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_C_NC);
            op = IA64_OP_LD1;
            break;
        case IA64_OP_LD1_S:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_S);
            op = IA64_OP_LD1;
            break;
        case IA64_OP_LD1_SA: 
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_SA);
            op = IA64_OP_LD1;
            break;
        case IA64_OP_LD16_:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_NONE);
            op = IA64_OP_LD16;
            break;
        case IA64_OP_LD16_ACQ:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_ACQ);
            op = IA64_OP_LD16;
            break;
        case IA64_OP_LD2_:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_NONE);
            op = IA64_OP_LD2;
            break;
        case IA64_OP_LD2_A:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_A);
            op = IA64_OP_LD2;
            break;
        case IA64_OP_LD2_ACQ:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_ACQ);
            op = IA64_OP_LD2;
            break;
        case IA64_OP_LD2_BIAS:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_BIAS);
            op = IA64_OP_LD2;
            break;
        case IA64_OP_LD2_C_CLR:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_C_CLR);
            op = IA64_OP_LD2;
            break;
        case IA64_OP_LD2_C_CLR_ACQ:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_C_CLR_ACQ);
            op = IA64_OP_LD2;
            break;
        case IA64_OP_LD2_C_NC:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_C_NC);
            op = IA64_OP_LD2;
            break;
        case IA64_OP_LD2_S:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_S);
            op = IA64_OP_LD2;
            break;
        case IA64_OP_LD2_SA: 
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_SA);
            op = IA64_OP_LD2;
            break;
        case IA64_OP_LD4_:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_NONE);
            op = IA64_OP_LD4;
            break;
        case IA64_OP_LD4_A:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_A);
            op = IA64_OP_LD4;
            break;
        case IA64_OP_LD4_ACQ:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_ACQ);
            op = IA64_OP_LD4;
            break;
        case IA64_OP_LD4_BIAS:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_BIAS);
            op = IA64_OP_LD4;
            break;
        case IA64_OP_LD4_C_CLR:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_C_CLR);
            op = IA64_OP_LD4;
            break;
        case IA64_OP_LD4_C_CLR_ACQ:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_C_CLR_ACQ);
            op = IA64_OP_LD4;
            break;
        case IA64_OP_LD4_C_NC:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_C_NC);
            op = IA64_OP_LD4;
            break;
        case IA64_OP_LD4_S:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_S);
            op = IA64_OP_LD4;
            break;
        case IA64_OP_LD4_SA: 
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_SA);
            op = IA64_OP_LD4;
            break;
        case IA64_OP_LD8_:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_NONE);
            op = IA64_OP_LD8;
            break;
        case IA64_OP_LD8_A:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_A);
            op = IA64_OP_LD8;
            break;
        case IA64_OP_LD8_ACQ:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_ACQ);
            op = IA64_OP_LD8;
            break;
        case IA64_OP_LD8_BIAS:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_BIAS);
            op = IA64_OP_LD8;
            break;
        case IA64_OP_LD8_C_CLR:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_C_CLR);
            op = IA64_OP_LD8;
            break;
        case IA64_OP_LD8_C_CLR_ACQ:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_C_CLR_ACQ);
            op = IA64_OP_LD8;
            break;
        case IA64_OP_LD8_C_NC:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_C_NC);
            op = IA64_OP_LD8;
            break;
        case IA64_OP_LD8_FILL:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_FILL);
            op = IA64_OP_LD8;
            break;
        case IA64_OP_LD8_S:
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_S);
            op = IA64_OP_LD8;
            break;
        case IA64_OP_LD8_SA: 
            ia64_cmpltr_add(i, IA64_CC_LDTYPE, IA64_CT_SA);
            op = IA64_OP_LD8;
            break;
        case IA64_OP_LDF_FILL:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_FILL);
            op = IA64_OP_LDF;
            break;
        case IA64_OP_LDF8_:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_NONE);
            op = IA64_OP_LDF8;
            break;
        case IA64_OP_LDF8_A:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_A);
            op = IA64_OP_LDF8;
            break;
        case IA64_OP_LDF8_C_CLR:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_C_CLR);
            op = IA64_OP_LDF8;
            break;
        case IA64_OP_LDF8_C_NC:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_C_NC);
            op = IA64_OP_LDF8;
            break;
        case IA64_OP_LDF8_S:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_S);
            op = IA64_OP_LDF8;
            break;
        case IA64_OP_LDF8_SA:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_SA);
            op = IA64_OP_LDF8;
            break;
        case IA64_OP_LDFD_:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_NONE);
            op = IA64_OP_LDFD;
            break;
        case IA64_OP_LDFD_A:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_A);
            op = IA64_OP_LDFD;
            break;
        case IA64_OP_LDFD_C_CLR:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_C_CLR);
            op = IA64_OP_LDFD;
            break;
        case IA64_OP_LDFD_C_NC:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_C_NC);
            op = IA64_OP_LDFD;
            break;
        case IA64_OP_LDFD_S:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_S);
            op = IA64_OP_LDFD;
            break;
        case IA64_OP_LDFD_SA:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_SA);
            op = IA64_OP_LDFD;
            break;
        case IA64_OP_LDFE_:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_NONE);
            op = IA64_OP_LDFE;
            break;
        case IA64_OP_LDFE_A:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_A);
            op = IA64_OP_LDFE;
            break;
        case IA64_OP_LDFE_C_CLR:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_C_CLR);
            op = IA64_OP_LDFE;
            break;
        case IA64_OP_LDFE_C_NC:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_C_NC);
            op = IA64_OP_LDFE;
            break;
        case IA64_OP_LDFE_S:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_S);
            op = IA64_OP_LDFE;
            break;
        case IA64_OP_LDFE_SA:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_SA);
            op = IA64_OP_LDFE;
            break;
        case IA64_OP_LDFP8_:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_NONE);
            op = IA64_OP_LDFP8;
            break;
        case IA64_OP_LDFP8_A:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_A);
            op = IA64_OP_LDFP8;
            break;
        case IA64_OP_LDFP8_C_CLR:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_C_CLR);
            op = IA64_OP_LDFP8;
            break;
        case IA64_OP_LDFP8_C_NC:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_C_NC);
            op = IA64_OP_LDFP8;
            break;
        case IA64_OP_LDFP8_S:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_S);
            op = IA64_OP_LDFP8;
            break;
        case IA64_OP_LDFP8_SA:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_SA);
            op = IA64_OP_LDFP8;
            break;
        case IA64_OP_LDFPD_:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_NONE);
            op = IA64_OP_LDFPD;
            break;
        case IA64_OP_LDFPD_A:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_A);
            op = IA64_OP_LDFPD;
            break;
        case IA64_OP_LDFPD_C_CLR:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_C_CLR);
            op = IA64_OP_LDFPD;
            break;
        case IA64_OP_LDFPD_C_NC:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_C_NC);
            op = IA64_OP_LDFPD;
            break;
        case IA64_OP_LDFPD_S:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_S);
            op = IA64_OP_LDFPD;
            break;
        case IA64_OP_LDFPD_SA:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_SA);
            op = IA64_OP_LDFPD;
            break;
        case IA64_OP_LDFPS_:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_NONE);
            op = IA64_OP_LDFPS;
            break;
        case IA64_OP_LDFPS_A:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_A);
            op = IA64_OP_LDFPS;
            break;
        case IA64_OP_LDFPS_C_CLR:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_C_CLR);
            op = IA64_OP_LDFPS;
            break;
        case IA64_OP_LDFPS_C_NC:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_C_NC);
            op = IA64_OP_LDFPS;
            break;
        case IA64_OP_LDFPS_S:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_S);
            op = IA64_OP_LDFPS;
            break;
        case IA64_OP_LDFPS_SA:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_SA);
            op = IA64_OP_LDFPS;
            break;
        case IA64_OP_LDFS_:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_NONE);
            op = IA64_OP_LDFS;
            break;
        case IA64_OP_LDFS_A:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_A);
            op = IA64_OP_LDFS;
            break;
        case IA64_OP_LDFS_C_CLR:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_C_CLR);
            op = IA64_OP_LDFS;
            break;
        case IA64_OP_LDFS_C_NC:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_C_NC);
            op = IA64_OP_LDFS;
            break;
        case IA64_OP_LDFS_S:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_S);
            op = IA64_OP_LDFS;
            break;
        case IA64_OP_LDFS_SA:
            ia64_cmpltr_add(i, IA64_CC_FLDTYPE, IA64_CT_SA);
            op = IA64_OP_LDFS;
            break;
        case IA64_OP_LFETCH_:
            ia64_cmpltr_add(i, IA64_CC_LFTYPE, IA64_CT_NONE);
            ia64_cmpltr_add(i, IA64_CC_LFETCH, IA64_CT_NONE);
            op = IA64_OP_LFETCH;
            break;
        case IA64_OP_LFETCH_EXCL:
            ia64_cmpltr_add(i, IA64_CC_LFTYPE, IA64_CT_NONE);
            ia64_cmpltr_add(i, IA64_CC_LFETCH, IA64_CT_EXCL);
            op = IA64_OP_LFETCH;
            break;
        case IA64_OP_LFETCH_FAULT:
            ia64_cmpltr_add(i, IA64_CC_LFTYPE, IA64_CT_FAULT);
            ia64_cmpltr_add(i, IA64_CC_LFETCH, IA64_CT_NONE);
            op = IA64_OP_LFETCH;
            break;
        case IA64_OP_LFETCH_FAULT_EXCL:
            ia64_cmpltr_add(i, IA64_CC_LFTYPE, IA64_CT_FAULT);
            ia64_cmpltr_add(i, IA64_CC_LFETCH, IA64_CT_EXCL);
            op = IA64_OP_LFETCH;
            break;
        case IA64_OP_MF_:
            ia64_cmpltr_add(i, IA64_CC_MF, IA64_CT_NONE);
            op = IA64_OP_MF;
            break;
        case IA64_OP_MF_A:
            ia64_cmpltr_add(i, IA64_CC_MF, IA64_CT_A);
            op = IA64_OP_MF;
            break;
        case IA64_OP_MIX1_L:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_L);
            op = IA64_OP_MIX1;
            break;
        case IA64_OP_MIX1_R:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_R);
            op = IA64_OP_MIX1;
            break;
        case IA64_OP_MIX2_L:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_L);
            op = IA64_OP_MIX2;
            break;
        case IA64_OP_MIX2_R:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_R);
            op = IA64_OP_MIX2;
            break;
        case IA64_OP_MIX4_L:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_L);
            op = IA64_OP_MIX4;
            break;
        case IA64_OP_MIX4_R:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_R);
            op = IA64_OP_MIX4;
            break;
        case IA64_OP_MOV_:
            ia64_cmpltr_add(i, IA64_CC_MOV, IA64_CT_NONE);
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_I:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_I);
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_M:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_M);
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_RET:
            ia64_cmpltr_add(i, IA64_CC_MOV, IA64_CT_RET);
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_CPUID:
            ot = IA64_OPER_CPUID;
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_DBR:
            ot = IA64_OPER_DBR;
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_IBR:
            ot = IA64_OPER_IBR;
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_IP:
            ot = IA64_OPER_IP;
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_MSR:
            ot = IA64_OPER_MSR;
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_PKR:
            ot = IA64_OPER_PKR;
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_PMC:
            ot = IA64_OPER_PMC;
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_PMD:
            ot = IA64_OPER_PMD;
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_PR:
            ot = IA64_OPER_PR;
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_PSR:
            ot = IA64_OPER_PSR;
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_PSR_L:
            ot = IA64_OPER_PSR_L;
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_PSR_UM:
            ot = IA64_OPER_PSR_UM;
            op = IA64_OP_MOV;
            break;
        case IA64_OP_MOV_RR:
            ot = IA64_OPER_RR;
            op = IA64_OP_MOV;
            break;
        case IA64_OP_NOP_B:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_B);
            op = IA64_OP_NOP;
            break;
        case IA64_OP_NOP_F:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_F);
            op = IA64_OP_NOP;
            break;
        case IA64_OP_NOP_I:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_I);
            op = IA64_OP_NOP;
            break;
        case IA64_OP_NOP_M:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_M);
            op = IA64_OP_NOP;
            break;
        case IA64_OP_NOP_X:
            ia64_cmpltr_add(i, IA64_CC_UNIT, IA64_CT_X);
            op = IA64_OP_NOP;
            break;
        case IA64_OP_PACK2_SSS:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_SSS);
            op = IA64_OP_PACK2;
            break;
        case IA64_OP_PACK2_USS:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_USS);
            op = IA64_OP_PACK2;
            break;
        case IA64_OP_PACK4_SSS:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_SSS);
            op = IA64_OP_PACK4;
            break;
        case IA64_OP_PADD1_:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_NONE);
            op = IA64_OP_PADD1;
            break;
        case IA64_OP_PADD1_SSS:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_SSS);
            op = IA64_OP_PADD1;
            break;
        case IA64_OP_PADD1_UUS:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_UUS);
            op = IA64_OP_PADD1;
            break;
        case IA64_OP_PADD1_UUU:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_UUU);
            op = IA64_OP_PADD1;
            break;
        case IA64_OP_PADD2_:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_NONE);
            op = IA64_OP_PADD2;
            break;
        case IA64_OP_PADD2_SSS:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_SSS);
            op = IA64_OP_PADD2;
            break;
        case IA64_OP_PADD2_UUS:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_UUS);
            op = IA64_OP_PADD2;
            break;
        case IA64_OP_PADD2_UUU:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_UUU);
            op = IA64_OP_PADD2;
            break;
        case IA64_OP_PAVG1_:
            ia64_cmpltr_add(i, IA64_CC_PAVG, IA64_CT_NONE);
            op = IA64_OP_PAVG1;
            break;
        case IA64_OP_PAVG1_RAZ:
            ia64_cmpltr_add(i, IA64_CC_PAVG, IA64_CT_RAZ);
            op = IA64_OP_PAVG1;
            break;
        case IA64_OP_PAVG2_:
            ia64_cmpltr_add(i, IA64_CC_PAVG, IA64_CT_NONE);
            op = IA64_OP_PAVG2;
            break;
        case IA64_OP_PAVG2_RAZ:
            ia64_cmpltr_add(i, IA64_CC_PAVG, IA64_CT_RAZ);
            op = IA64_OP_PAVG2;
            break;
        case IA64_OP_PCMP1_EQ:
            ia64_cmpltr_add(i, IA64_CC_PREL, IA64_CT_EQ);
            op = IA64_OP_PCMP1;
            break;
        case IA64_OP_PCMP1_GT:
            ia64_cmpltr_add(i, IA64_CC_PREL, IA64_CT_GT);
            op = IA64_OP_PCMP1;
            break;
        case IA64_OP_PCMP2_EQ:
            ia64_cmpltr_add(i, IA64_CC_PREL, IA64_CT_EQ);
            op = IA64_OP_PCMP2;
            break;
        case IA64_OP_PCMP2_GT:
            ia64_cmpltr_add(i, IA64_CC_PREL, IA64_CT_GT);
            op = IA64_OP_PCMP2;
            break;
        case IA64_OP_PCMP4_EQ:
            ia64_cmpltr_add(i, IA64_CC_PREL, IA64_CT_EQ);
            op = IA64_OP_PCMP4;
            break;
        case IA64_OP_PCMP4_GT:
            ia64_cmpltr_add(i, IA64_CC_PREL, IA64_CT_GT);
            op = IA64_OP_PCMP4;
            break;
        case IA64_OP_PMAX1_U:
            ia64_cmpltr_add(i, IA64_CC_UNS, IA64_CT_U);
            op = IA64_OP_PMAX1;
            break;
        case IA64_OP_PMIN1_U:
            ia64_cmpltr_add(i, IA64_CC_UNS, IA64_CT_U);
            op = IA64_OP_PMIN1;
            break;
        case IA64_OP_PMPY2_L:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_L);
            op = IA64_OP_PMPY2;
            break;
        case IA64_OP_PMPY2_R:
            ia64_cmpltr_add(i, IA64_CC_LR, IA64_CT_R);
            op = IA64_OP_PMPY2;
            break;
        case IA64_OP_PMPYSHR2_:
            ia64_cmpltr_add(i, IA64_CC_UNS, IA64_CT_NONE);
            op = IA64_OP_PMPYSHR2;
            break;
        case IA64_OP_PMPYSHR2_U:
            ia64_cmpltr_add(i, IA64_CC_UNS, IA64_CT_U);
            op = IA64_OP_PMPYSHR2;
            break;
        case IA64_OP_PROBE_R:
            ia64_cmpltr_add(i, IA64_CC_RW, IA64_CT_R);
            ia64_cmpltr_add(i, IA64_CC_PRTYPE, IA64_CT_NONE);
            op = IA64_OP_PROBE;
            break;
        case IA64_OP_PROBE_R_FAULT:
            ia64_cmpltr_add(i, IA64_CC_RW, IA64_CT_R);
            ia64_cmpltr_add(i, IA64_CC_PRTYPE, IA64_CT_FAULT);
            op = IA64_OP_PROBE;
            break;
        case IA64_OP_PROBE_RW_FAULT:
            ia64_cmpltr_add(i, IA64_CC_RW, IA64_CT_RW);
            ia64_cmpltr_add(i, IA64_CC_PRTYPE, IA64_CT_FAULT);
            op = IA64_OP_PROBE;
            break;
        case IA64_OP_PROBE_W:
            ia64_cmpltr_add(i, IA64_CC_RW, IA64_CT_W);
            ia64_cmpltr_add(i, IA64_CC_PRTYPE, IA64_CT_NONE);
            op = IA64_OP_PROBE;
            break;
        case IA64_OP_PROBE_W_FAULT:
            ia64_cmpltr_add(i, IA64_CC_RW, IA64_CT_W);
            ia64_cmpltr_add(i, IA64_CC_PRTYPE, IA64_CT_FAULT);
            op = IA64_OP_PROBE;
            break;
        case IA64_OP_PSHR2_:
            ia64_cmpltr_add(i, IA64_CC_UNS, IA64_CT_NONE);
            op = IA64_OP_PSHR2;
            break;
        case IA64_OP_PSHR2_U:
            ia64_cmpltr_add(i, IA64_CC_UNS, IA64_CT_U);
            op = IA64_OP_PSHR2;
            break;
        case IA64_OP_PSHR4_:
            ia64_cmpltr_add(i, IA64_CC_UNS, IA64_CT_NONE);
            op = IA64_OP_PSHR4;
            break;
        case IA64_OP_PSHR4_U:
            ia64_cmpltr_add(i, IA64_CC_UNS, IA64_CT_U);
            op = IA64_OP_PSHR4;
            break;
        case IA64_OP_PSUB1_:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_NONE);
            op = IA64_OP_PSUB1;
            break;
        case IA64_OP_PSUB1_SSS:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_SSS);
            op = IA64_OP_PSUB1;
            break;
        case IA64_OP_PSUB1_UUS:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_UUS);
            op = IA64_OP_PSUB1;
            break;
        case IA64_OP_PSUB1_UUU:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_UUU);
            op = IA64_OP_PSUB1;
            break;
        case IA64_OP_PSUB2_:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_NONE);
            op = IA64_OP_PSUB2;
            break;
        case IA64_OP_PSUB2_SSS:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_SSS);
            op = IA64_OP_PSUB2;
            break;
        case IA64_OP_PSUB2_UUS:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_UUS);
            op = IA64_OP_PSUB2;
            break;
        case IA64_OP_PSUB2_UUU:
            ia64_cmpltr_add(i, IA64_CC_SAT, IA64_CT_UUU);
            op = IA64_OP_PSUB2;
            break;
        case IA64_OP_PTC_E:
            ia64_cmpltr_add(i, IA64_CC_PTC, IA64_CT_E);
            op = IA64_OP_PTC;
            break;
        case IA64_OP_PTC_G:
            ia64_cmpltr_add(i, IA64_CC_PTC, IA64_CT_G);
            op = IA64_OP_PTC;
            break;
        case IA64_OP_PTC_GA:
            ia64_cmpltr_add(i, IA64_CC_PTC, IA64_CT_GA);
            op = IA64_OP_PTC;
            break;
        case IA64_OP_PTC_L:
            ia64_cmpltr_add(i, IA64_CC_PTC, IA64_CT_L);
            op = IA64_OP_PTC;
            break;
        case IA64_OP_PTR_D:
            ia64_cmpltr_add(i, IA64_CC_PTR, IA64_CT_D);
            op = IA64_OP_PTR;
            break;
        case IA64_OP_PTR_I:
            ia64_cmpltr_add(i, IA64_CC_PTR, IA64_CT_I);
            op = IA64_OP_PTR;
            break;
        case IA64_OP_SETF_D:
            ia64_cmpltr_add(i, IA64_CC_SETF, IA64_CT_D);
            op = IA64_OP_SETF;
            break;
        case IA64_OP_SETF_EXP:
            ia64_cmpltr_add(i, IA64_CC_SETF, IA64_CT_EXP);
            op = IA64_OP_SETF;
            break;
        case IA64_OP_SETF_S:
            ia64_cmpltr_add(i, IA64_CC_SETF, IA64_CT_S);
            op = IA64_OP_SETF;
            break;
        case IA64_OP_SETF_SIG:
            ia64_cmpltr_add(i, IA64_CC_SETF, IA64_CT_SIG);
            op = IA64_OP_SETF;
            break;
        case IA64_OP_SHR_:
            ia64_cmpltr_add(i, IA64_CC_UNS, IA64_CT_NONE);
            op = IA64_OP_SHR;
            break;
        case IA64_OP_SHR_U:
            ia64_cmpltr_add(i, IA64_CC_UNS, IA64_CT_U);
            op = IA64_OP_SHR;
            break;
        case IA64_OP_SRLZ_D:
            ia64_cmpltr_add(i, IA64_CC_SRLZ, IA64_CT_D);
            op = IA64_OP_SRLZ;
            break;
        case IA64_OP_SRLZ_I:
            ia64_cmpltr_add(i, IA64_CC_SRLZ, IA64_CT_I);
            op = IA64_OP_SRLZ;
            break;
        case IA64_OP_ST1_:
            ia64_cmpltr_add(i, IA64_CC_STTYPE, IA64_CT_NONE);
            op = IA64_OP_ST1;
            break;
        case IA64_OP_ST1_REL:
            ia64_cmpltr_add(i, IA64_CC_STTYPE, IA64_CT_REL);
            op = IA64_OP_ST1;
            break;
        case IA64_OP_ST16_:
            ia64_cmpltr_add(i, IA64_CC_STTYPE, IA64_CT_NONE);
            op = IA64_OP_ST16;
            break;
        case IA64_OP_ST16_REL:
            ia64_cmpltr_add(i, IA64_CC_STTYPE, IA64_CT_REL);
            op = IA64_OP_ST16;
            break;
        case IA64_OP_ST2_:
            ia64_cmpltr_add(i, IA64_CC_STTYPE, IA64_CT_NONE);
            op = IA64_OP_ST2;
            break;
        case IA64_OP_ST2_REL:
            ia64_cmpltr_add(i, IA64_CC_STTYPE, IA64_CT_REL);
            op = IA64_OP_ST2;
            break;
        case IA64_OP_ST4_:
            ia64_cmpltr_add(i, IA64_CC_STTYPE, IA64_CT_NONE);
            op = IA64_OP_ST4;
            break;
        case IA64_OP_ST4_REL:
            ia64_cmpltr_add(i, IA64_CC_STTYPE, IA64_CT_REL);
            op = IA64_OP_ST4;
            break;
        case IA64_OP_ST8_:
            ia64_cmpltr_add(i, IA64_CC_STTYPE, IA64_CT_NONE);
            op = IA64_OP_ST8;
            break;
        case IA64_OP_ST8_REL:
            ia64_cmpltr_add(i, IA64_CC_STTYPE, IA64_CT_REL);
            op = IA64_OP_ST8;
            break;
        case IA64_OP_ST8_SPILL:
            ia64_cmpltr_add(i, IA64_CC_STTYPE, IA64_CT_SPILL);
            op = IA64_OP_ST8;
            break;
        case IA64_OP_STF_SPILL:
            ia64_cmpltr_add(i, IA64_CC_STTYPE, IA64_CT_SPILL);
            op = IA64_OP_STF;
            break;
        case IA64_OP_SYNC_I:
            ia64_cmpltr_add(i, IA64_CC_SYNC, IA64_CT_I);
            op = IA64_OP_SYNC;
            break;
        case IA64_OP_TBIT_NZ_AND:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_NZ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_TBIT;
            break;
        case IA64_OP_TBIT_NZ_OR:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_NZ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_TBIT;
            break;
        case IA64_OP_TBIT_NZ_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_NZ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_TBIT;
            break;
        case IA64_OP_TBIT_Z:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_NONE);
            op = IA64_OP_TBIT;
            break;
        case IA64_OP_TBIT_Z_AND:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_TBIT;
            break;
        case IA64_OP_TBIT_Z_OR:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_TBIT;
            break;
        case IA64_OP_TBIT_Z_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_TBIT;
            break;
        case IA64_OP_TBIT_Z_UNC:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_UNC);
            op = IA64_OP_TBIT;
            break;
        case IA64_OP_TF_NZ_AND:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_NZ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_TF;
            break;
        case IA64_OP_TF_NZ_OR:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_NZ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_TF;
            break;
        case IA64_OP_TF_NZ_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_NZ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_TF;
            break;
        case IA64_OP_TF_Z:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_NONE);
            op = IA64_OP_TF;
            break;
        case IA64_OP_TF_Z_AND:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_TF;
            break;
        case IA64_OP_TF_Z_OR:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_TF;
            break;
        case IA64_OP_TF_Z_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_TF;
            break;
        case IA64_OP_TF_Z_UNC:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_UNC);
            op = IA64_OP_TF;
            break;
        case IA64_OP_TNAT_NZ_AND:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_NZ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_TNAT;
            break;
        case IA64_OP_TNAT_NZ_OR:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_NZ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_TNAT;
            break;
        case IA64_OP_TNAT_NZ_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_NZ);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_TNAT;
            break;
        case IA64_OP_TNAT_Z:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_NONE);
            op = IA64_OP_TNAT;
            break;
        case IA64_OP_TNAT_Z_AND:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_AND);
            op = IA64_OP_TNAT;
            break;
        case IA64_OP_TNAT_Z_OR:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR);
            op = IA64_OP_TNAT;
            break;
        case IA64_OP_TNAT_Z_OR_ANDCM:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_OR_ANDCM);
            op = IA64_OP_TNAT;
            break;
        case IA64_OP_TNAT_Z_UNC:
            ia64_cmpltr_add(i, IA64_CC_TREL, IA64_CT_Z);
            ia64_cmpltr_add(i, IA64_CC_CTYPE, IA64_CT_UNC);
            op = IA64_OP_TNAT;
            break;
        case IA64_OP_UNPACK1_H:
            ia64_cmpltr_add(i, IA64_CC_UNPACK, IA64_CT_H);
            op = IA64_OP_UNPACK1;
            break;
        case IA64_OP_UNPACK1_L:
            ia64_cmpltr_add(i, IA64_CC_UNPACK, IA64_CT_L);
            op = IA64_OP_UNPACK1;
            break;
        case IA64_OP_UNPACK2_H:
            ia64_cmpltr_add(i, IA64_CC_UNPACK, IA64_CT_H);
            op = IA64_OP_UNPACK2;
            break;
        case IA64_OP_UNPACK2_L:
            ia64_cmpltr_add(i, IA64_CC_UNPACK, IA64_CT_L);
            op = IA64_OP_UNPACK2;
            break;
        case IA64_OP_UNPACK4_H:
            ia64_cmpltr_add(i, IA64_CC_UNPACK, IA64_CT_H);
            op = IA64_OP_UNPACK4;
            break;
        case IA64_OP_UNPACK4_L:
            ia64_cmpltr_add(i, IA64_CC_UNPACK, IA64_CT_L);
            op = IA64_OP_UNPACK4;
            break;
        case IA64_OP_VMSW_0:
            ia64_cmpltr_add(i, IA64_CC_VMSW, IA64_CT_0);
            op = IA64_OP_VMSW;
            break;
        case IA64_OP_VMSW_1:
            ia64_cmpltr_add(i, IA64_CC_VMSW, IA64_CT_1);
            op = IA64_OP_VMSW;
            break;
        case IA64_OP_XMA_H:
            ia64_cmpltr_add(i, IA64_CC_XMA, IA64_CT_H);
            op = IA64_OP_XMA;
            break;
        case IA64_OP_XMA_HU:
            ia64_cmpltr_add(i, IA64_CC_XMA, IA64_CT_HU);
            op = IA64_OP_XMA;
            break;
        case IA64_OP_XMA_L:
            ia64_cmpltr_add(i, IA64_CC_XMA, IA64_CT_L);
            op = IA64_OP_XMA;
            break;
        default:
            assert(op < IA64_OP_NUMBER_OF_OPCODES);
            break;
	}
	i->i_op = op;
	return (ot);
}

static __inline void
op_imm(struct ia64_inst *i, int op, uint64_t val)
{
	i->i_oper[op].o_type = IA64_OPER_IMM;
	i->i_oper[op].o_value = val;
}

static __inline void
op_type(struct ia64_inst *i, int op, enum ia64_oper_type ot)
{
	i->i_oper[op].o_type = ot;
}

static __inline void
op_value(struct ia64_inst *i, int op, uint64_t val)
{
	i->i_oper[op].o_value = val;
}

static __inline void
operand(struct ia64_inst *i, int op, enum ia64_oper_type ot, uint64_t bits,
        int o, int l)
{
	i->i_oper[op].o_type = ot;
	i->i_oper[op].o_value = FIELD(bits, o, l);
}

static uint64_t
imm(uint64_t bits, int sign, int o, int l)
{
	uint64_t val = FIELD(bits, o, l);
    
	if (sign && (val & (1LL << (l - 1))) != 0)
		val |= -1LL << l;
	return (val);
}

static void
s_imm(struct ia64_inst *i, int op, uint64_t bits, int o, int l)
{
	i->i_oper[op].o_type = IA64_OPER_IMM;
	i->i_oper[op].o_value = imm(bits, 1, o, l);
}

static void
u_imm(struct ia64_inst *i, int op, uint64_t bits, int o, int l)
{
	i->i_oper[op].o_type = IA64_OPER_IMM;
	i->i_oper[op].o_value = imm(bits, 0, o, l);
}

static uint64_t
vimm(uint64_t bits, int sign, va_list ap)
{
	uint64_t val = 0;
	int len = 0;
	int frag;
    
	while ((frag = va_arg(ap, int)) != 0) {
		val |= (uint64_t)FIELD(bits, FRAG_OFS(frag), FRAG_LEN(frag))
        << len;
		len += FRAG_LEN(frag);
	}
	if (sign && (val & (1LL << (len - 1))) != 0)
		val |= -1LL << len;
	return (val);
}

static void
s_immf(struct ia64_inst *i, int op, uint64_t bits, ...)
{
	va_list ap;
	va_start(ap, bits);
	i->i_oper[op].o_type = IA64_OPER_IMM;
	i->i_oper[op].o_value = vimm(bits, 1, ap);
	va_end(ap);
}

static void
u_immf(struct ia64_inst *i, int op, uint64_t bits, ...)
{
	va_list ap;
	va_start(ap, bits);
	i->i_oper[op].o_type = IA64_OPER_IMM;
	i->i_oper[op].o_value = vimm(bits, 0, ap);
	va_end(ap);
}

static void
disp(struct ia64_inst *i, int op, uint64_t bits, ...)
{
	va_list ap;
	va_start(ap, bits);
	i->i_oper[op].o_type = IA64_OPER_DISP;
	i->i_oper[op].o_value = vimm(bits, 1, ap) << 4;
	va_end(ap);
}

static __inline void
combine(uint64_t *dst, int dl, uint64_t src, int sl, int so)
{
	*dst = (*dst & ((1LL << dl) - 1LL)) |
    ((uint64_t)_FLD64(src, so, sl) << dl);
}

int
ia64_extract(enum ia64_op op, enum ia64_fmt fmt, uint64_t bits,
             struct ia64_bundle *b, int slot)
{
	struct ia64_inst *i = b->b_inst + slot;
	enum ia64_oper_type ot;
    
	assert(op != IA64_OP_NONE);
	i->i_bits = bits;
	i->i_format = fmt;
	i->i_srcidx = 2;
    
	ot = ia64_normalize(i, op);
    
	if (fmt != IA64_FMT_B6 && fmt != IA64_FMT_B7)
		operand(i, 0, IA64_OPER_PREG, bits, 0, 6);
    
	switch (fmt) {
        case IA64_FMT_A1:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            operand(i, 3, IA64_OPER_GREG, bits, 20, 7);
            if ((op == IA64_OP_ADD && FIELD(bits, 27, 2) == 1) ||
                (op == IA64_OP_SUB && FIELD(bits, 27, 2) == 0))
                op_imm(i, 4, 1LL);
            break;
        case IA64_FMT_A2:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            op_imm(i, 3, 1LL + FIELD(bits, 27, 2));
            operand(i, 4, IA64_OPER_GREG, bits, 20, 7);
            break;
        case IA64_FMT_A3:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            s_immf(i, 2, bits, FRAG(13,7), FRAG(36,1), 0);
            operand(i, 3, IA64_OPER_GREG, bits, 20, 7);
            break;
        case IA64_FMT_A4:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            s_immf(i, 2, bits, FRAG(13,7), FRAG(27,6), FRAG(36,1), 0);
            operand(i, 3, IA64_OPER_GREG, bits, 20, 7);
            break;
        case IA64_FMT_A5:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            s_immf(i, 2, bits, FRAG(13,7), FRAG(27,9), FRAG(22,5),
                   FRAG(36,1), 0);
            operand(i, 3, IA64_OPER_GREG, bits, 20, 2);
            break;
        case IA64_FMT_A6: /* 2 dst */
            operand(i, 1, IA64_OPER_PREG, bits, 6, 6);
            operand(i, 2, IA64_OPER_PREG, bits, 27, 6);
            operand(i, 3, IA64_OPER_GREG, bits, 13, 7);
            operand(i, 4, IA64_OPER_GREG, bits, 20, 7);
            i->i_srcidx++;
            break;
        case IA64_FMT_A7: /* 2 dst */
            if (FIELD(bits, 13, 7) != 0)
                return (0);
            operand(i, 1, IA64_OPER_PREG, bits, 6, 6);
            operand(i, 2, IA64_OPER_PREG, bits, 27, 6);
            operand(i, 3, IA64_OPER_GREG, bits, 13, 7);
            operand(i, 4, IA64_OPER_GREG, bits, 20, 7);
            i->i_srcidx++;
            break;
        case IA64_FMT_A8: /* 2 dst */
            operand(i, 1, IA64_OPER_PREG, bits, 6, 6);
            operand(i, 2, IA64_OPER_PREG, bits, 27, 6);
            s_immf(i, 3, bits, FRAG(13,7), FRAG(36,1), 0);
            operand(i, 4, IA64_OPER_GREG, bits, 20, 7);
            i->i_srcidx++;
            break;
        case IA64_FMT_A9:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            operand(i, 3, IA64_OPER_GREG, bits, 20, 7);
            break;
        case IA64_FMT_A10:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            op_imm(i, 3, 1LL + FIELD(bits, 27, 2));
            operand(i, 4, IA64_OPER_GREG, bits, 20, 7);
            break;
        case IA64_FMT_B1: /* 0 dst */
            ia64_brhint(i);
            disp(i, 1, bits, FRAG(13,20), FRAG(36,1), 0);
            break;
        case IA64_FMT_B2: /* 0 dst */
            if (FIELD(bits, 0, 6) != 0)
                return (0);
            ia64_brhint(i);
            disp(i, 1, bits, FRAG(13,20), FRAG(36,1), 0);
            break;
        case IA64_FMT_B3:
            ia64_brhint(i);
            operand(i, 1, IA64_OPER_BREG, bits, 6, 3);
            disp(i, 2, bits, FRAG(13,20), FRAG(36,1), 0);
            break;
        case IA64_FMT_B4: /* 0 dst */
            ia64_brhint(i);
            operand(i, 1, IA64_OPER_BREG, bits, 13, 3);
            break;
        case IA64_FMT_B5:
#if 0
            if (FIELD(bits, 32, 1) == 0)
                return (0);
#endif
            ia64_brhint(i);
            operand(i, 1, IA64_OPER_BREG, bits, 6, 3);
            operand(i, 2, IA64_OPER_BREG, bits, 13, 3);
            break;
        case IA64_FMT_B6: /* 0 dst */
            ia64_brphint(i);
            disp(i, 1, bits, FRAG(13,20), FRAG(36,1), 0);
            disp(i, 2, bits, FRAG(6,7), FRAG(33,2), 0);
            i->i_srcidx--;
            break;
        case IA64_FMT_B7: /* 0 dst */
            ia64_brphint(i);
            operand(i, 1, IA64_OPER_BREG, bits, 13, 3);
            disp(i, 2, bits, FRAG(6,7), FRAG(33,2), 0);
            i->i_srcidx--;
            break;
        case IA64_FMT_B8:
            /* no operands */
            break;
        case IA64_FMT_B9: /* 0 dst */
            u_immf(i, 1, bits, FRAG(6,20), FRAG(36,1), 0);
            break;
        case IA64_FMT_F1:
            ia64_sf(i);
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_FREG, bits, 13, 7);
            operand(i, 3, IA64_OPER_FREG, bits, 20, 7);
            operand(i, 4, IA64_OPER_FREG, bits, 27, 7);
            break;
        case IA64_FMT_F2:
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_FREG, bits, 13, 7);
            operand(i, 3, IA64_OPER_FREG, bits, 20, 7);
            operand(i, 4, IA64_OPER_FREG, bits, 27, 7);
            break;
        case IA64_FMT_F3:
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_FREG, bits, 13, 7);
            operand(i, 3, IA64_OPER_FREG, bits, 20, 7);
            operand(i, 4, IA64_OPER_FREG, bits, 27, 7);
            break;
        case IA64_FMT_F4: /* 2 dst */
            if (FIELD(bits, 33, 1)) { /* ra */
                if (FIELD(bits, 36, 1)) /* rb */
                    ia64_cmpltr_add(i, IA64_CC_FREL, IA64_CT_UNORD);
                else
                    ia64_cmpltr_add(i, IA64_CC_FREL, IA64_CT_LE);
            } else {
                if (FIELD(bits, 36, 1)) /* rb */
                    ia64_cmpltr_add(i, IA64_CC_FREL, IA64_CT_LT);
                else
                    ia64_cmpltr_add(i, IA64_CC_FREL, IA64_CT_EQ);
            }
            if (FIELD(bits, 12, 1)) /* ta */
                ia64_cmpltr_add(i, IA64_CC_FCTYPE, IA64_CT_UNC);
            else
                ia64_cmpltr_add(i, IA64_CC_FCTYPE, IA64_CT_NONE);
            ia64_sf(i);
            operand(i, 1, IA64_OPER_PREG, bits, 6, 6);
            operand(i, 2, IA64_OPER_PREG, bits, 27, 6);
            operand(i, 3, IA64_OPER_FREG, bits, 13, 7);
            operand(i, 4, IA64_OPER_FREG, bits, 20, 7);
            i->i_srcidx++;
            break;
        case IA64_FMT_F5: /* 2 dst */
            operand(i, 1, IA64_OPER_PREG, bits, 6, 6);
            operand(i, 2, IA64_OPER_PREG, bits, 27, 6);
            operand(i, 3, IA64_OPER_FREG, bits, 13, 7);
            u_immf(i, 4, bits, FRAG(33,2), FRAG(20,7), 0);
            i->i_srcidx++;
            break;
        case IA64_FMT_F6: /* 2 dst */
            ia64_sf(i);
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_PREG, bits, 27, 6);
            operand(i, 3, IA64_OPER_FREG, bits, 13, 7);
            operand(i, 4, IA64_OPER_FREG, bits, 20, 7);
            i->i_srcidx++;
            break;
        case IA64_FMT_F7: /* 2 dst */
            ia64_sf(i);
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_PREG, bits, 27, 6);
            operand(i, 3, IA64_OPER_FREG, bits, 20, 7);
            i->i_srcidx++;
            break;
        case IA64_FMT_F8:
            ia64_sf(i);
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_FREG, bits, 13, 7);
            operand(i, 3, IA64_OPER_FREG, bits, 20, 7);
            break;
        case IA64_FMT_F9:
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_FREG, bits, 13, 7);
            operand(i, 3, IA64_OPER_FREG, bits, 20, 7);
            break;
        case IA64_FMT_F10:
            ia64_sf(i);
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_FREG, bits, 13, 7);
            break;
        case IA64_FMT_F11:
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_FREG, bits, 13, 7);
            break;
        case IA64_FMT_F12: /* 0 dst */
            ia64_sf(i);
            u_imm(i, 1, bits, 13, 7);
            u_imm(i, 2, bits, 20, 7);
            i->i_srcidx--;
            break;
        case IA64_FMT_F13:
            ia64_sf(i);
            /* no operands */
            break;
        case IA64_FMT_F14: /* 0 dst */
            ia64_sf(i);
            disp(i, 1, bits, FRAG(6,20), FRAG(36,1), 0);
            break;
        case IA64_FMT_F15: /* 0 dst */
            u_imm(i, 1, bits, 6, 20);
            break;
        case IA64_FMT_F16: /* 0 dst */
            u_imm(i, 1, bits, 6, 20);
            break;
        case IA64_FMT_I1:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            operand(i, 3, IA64_OPER_GREG, bits, 20, 7);
            switch (FIELD(bits, 30, 2)) {
                case 0:	op_imm(i, 4, 0LL); break;
                case 1: op_imm(i, 4, 7LL); break;
                case 2: op_imm(i, 4, 15LL); break;
                case 3: op_imm(i, 4, 16LL); break;
            }
            break;
        case IA64_FMT_I2:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            operand(i, 3, IA64_OPER_GREG, bits, 20, 7);
            break;
        case IA64_FMT_I3:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            u_imm(i, 3, bits, 20, 4);
            break;
        case IA64_FMT_I4:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            u_imm(i, 3, bits, 20, 8);
            break;
        case IA64_FMT_I5:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 20, 7);
            operand(i, 3, IA64_OPER_GREG, bits, 13, 7);
            break;
        case IA64_FMT_I6:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 20, 7);
            u_imm(i, 3, bits, 14, 5);
            break;
        case IA64_FMT_I7:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            operand(i, 3, IA64_OPER_GREG, bits, 20, 7);
            break;
        case IA64_FMT_I8:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            op_imm(i, 3, 31LL - FIELD(bits, 20, 5));
            break;
        case IA64_FMT_I9:
            if (FIELD(bits, 13, 7) != 0)
                return (0);
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 20, 7);
            break;
        case IA64_FMT_I10:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            operand(i, 3, IA64_OPER_GREG, bits, 20, 7);
            u_imm(i, 4, bits, 27, 6);
            break;
        case IA64_FMT_I11:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 20, 7);
            u_imm(i, 3, bits, 14, 6);
            op_imm(i, 4, 1LL + FIELD(bits, 27, 6));
            break;
        case IA64_FMT_I12:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            op_imm(i, 3, 63LL - FIELD(bits, 20, 6));
            op_imm(i, 4, 1LL + FIELD(bits, 27, 6));
            break;
        case IA64_FMT_I13:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            s_immf(i, 2, bits, FRAG(13,7), FRAG(36,1), 0);
            op_imm(i, 3, 63LL - FIELD(bits, 20, 6));
            op_imm(i, 4, 1LL + FIELD(bits, 27, 6));
            break;
        case IA64_FMT_I14:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            s_imm(i, 2, bits, 36, 1);
            operand(i, 3, IA64_OPER_GREG, bits, 20, 7);
            op_imm(i, 4, 63LL - FIELD(bits, 14, 6));
            op_imm(i, 5, 1LL + FIELD(bits, 27, 6));
            break;
        case IA64_FMT_I15:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            operand(i, 3, IA64_OPER_GREG, bits, 20, 7);
            op_imm(i, 4, 63LL - FIELD(bits, 31, 6));
            op_imm(i, 5, 1LL + FIELD(bits, 27, 4));
            break;
        case IA64_FMT_I16: /* 2 dst */
            operand(i, 1, IA64_OPER_PREG, bits, 6, 6);
            operand(i, 2, IA64_OPER_PREG, bits, 27, 6);
            operand(i, 3, IA64_OPER_GREG, bits, 20, 7);
            u_imm(i, 4, bits, 14, 6);
            i->i_srcidx++;
            break;
        case IA64_FMT_I17: /* 2 dst */
            operand(i, 1, IA64_OPER_PREG, bits, 6, 6);
            operand(i, 2, IA64_OPER_PREG, bits, 27, 6);
            operand(i, 3, IA64_OPER_GREG, bits, 20, 7);
            i->i_srcidx++;
            break;
        case IA64_FMT_I18:
            u_immf(i, 1, bits, FRAG(6,20), FRAG(36,1), 0);
            break;
        case IA64_FMT_I19:
            u_immf(i, 1, bits, FRAG(6,20), FRAG(36,1), 0);
            break;
        case IA64_FMT_I20: /* 0 dst */
            operand(i, 1, IA64_OPER_GREG, bits, 13, 7);
            disp(i, 2, bits, FRAG(6,7), FRAG(20,13), FRAG(36,1), 0);
            i->i_srcidx--;
            break;
        case IA64_FMT_I21:
            switch (FIELD(bits, 20, 2)) { /* wh */
                case 0:	ia64_cmpltr_add(i, IA64_CC_MWH, IA64_CT_SPTK); break;
                case 1:	ia64_cmpltr_add(i, IA64_CC_MWH, IA64_CT_NONE); break;
                case 2:	ia64_cmpltr_add(i, IA64_CC_MWH, IA64_CT_DPTK); break;
                case 3:	return (0);
            }
            if (FIELD(bits, 23, 1)) /* ih */
                ia64_cmpltr_add(i, IA64_CC_IH, IA64_CT_IMP);
            else
                ia64_cmpltr_add(i, IA64_CC_IH, IA64_CT_NONE);
            operand(i, 1, IA64_OPER_BREG, bits, 6, 3);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            disp(i, 3, bits, FRAG(24,9), 0);
            break;
        case IA64_FMT_I22:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_BREG, bits, 13, 3);
            break;
        case IA64_FMT_I23:
            op_type(i, 1, IA64_OPER_PR);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            u_immf(i, 3, bits, FRAG(6,7), FRAG(24,8), FRAG(36,1), 0);
            i->i_oper[3].o_value <<= 1;
            break;
        case IA64_FMT_I24:
            op_type(i, 1, IA64_OPER_PR_ROT);
            s_immf(i, 2, bits, FRAG(6,27), FRAG(36,1), 0);
            break;
        case IA64_FMT_I25:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            op_type(i, 2, ot);
            break;
        case IA64_FMT_I26:
            operand(i, 1, IA64_OPER_AREG, bits, 20, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            break;
        case IA64_FMT_I27:
            operand(i, 1, IA64_OPER_AREG, bits, 20, 7);
            s_immf(i, 2, bits, FRAG(13,7), FRAG(36,1), 0);
            break;
        case IA64_FMT_I28:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_AREG, bits, 20, 7);
            break;
        case IA64_FMT_I29:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 20, 7);
            break;
        case IA64_FMT_I30: /* 2 dst */
            operand(i, 1, IA64_OPER_PREG, bits, 6, 6);
            operand(i, 2, IA64_OPER_PREG, bits, 27, 6);
            op_imm(i, 3, 32LL + FIELD(bits, 14, 5));
            i->i_srcidx++;
            break;
        case IA64_FMT_M1:
            ia64_hint(i, IA64_CC_LDHINT);
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            if (i->i_op == IA64_OP_LD16) {
                op_type(i, 2, IA64_OPER_AREG);
                op_value(i, 2, AR_CSD);
                i->i_srcidx++;
            }
            operand(i, i->i_srcidx, IA64_OPER_MEM, bits, 20, 7);
            break;
        case IA64_FMT_M2:
            ia64_hint(i, IA64_CC_LDHINT);
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_MEM, bits, 20, 7);
            operand(i, 3, IA64_OPER_GREG, bits, 13, 7);
            break;
        case IA64_FMT_M3:
            ia64_hint(i, IA64_CC_LDHINT);
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_MEM, bits, 20, 7);
            s_immf(i, 3, bits, FRAG(13,7), FRAG(27,1), FRAG(36,1), 0);
            break;
        case IA64_FMT_M4:
            ia64_hint(i, IA64_CC_STHINT);
            operand(i, 1, IA64_OPER_MEM, bits, 20, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            if (i->i_op == IA64_OP_ST16) {
                op_type(i, 3, IA64_OPER_AREG);
                op_value(i, 3, AR_CSD);
            }
            break;
        case IA64_FMT_M5:
            ia64_hint(i, IA64_CC_STHINT);
            operand(i, 1, IA64_OPER_MEM, bits, 20, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            s_immf(i, 3, bits, FRAG(6,7), FRAG(27,1), FRAG(36,1), 0);
            break;
        case IA64_FMT_M6:
            ia64_hint(i, IA64_CC_LDHINT);
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_MEM, bits, 20, 7);
            break;
        case IA64_FMT_M7:
            ia64_hint(i, IA64_CC_LDHINT);
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_MEM, bits, 20, 7);
            operand(i, 3, IA64_OPER_GREG, bits, 13, 7);
            break;
        case IA64_FMT_M8:
            ia64_hint(i, IA64_CC_LDHINT);
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_MEM, bits, 20, 7);
            s_immf(i, 3, bits, FRAG(13,7), FRAG(27,1), FRAG(36,1), 0);
            break;
        case IA64_FMT_M9:
            ia64_hint(i, IA64_CC_STHINT);
            operand(i, 1, IA64_OPER_MEM, bits, 20, 7);
            operand(i, 2, IA64_OPER_FREG, bits, 13, 7);
            break;
        case IA64_FMT_M10:
            ia64_hint(i, IA64_CC_STHINT);
            operand(i, 1, IA64_OPER_MEM, bits, 20, 7);
            operand(i, 2, IA64_OPER_FREG, bits, 13, 7);
            s_immf(i, 3, bits, FRAG(6,7), FRAG(27,1), FRAG(36,1), 0);
            break;
        case IA64_FMT_M11: /* 2 dst */
            ia64_hint(i, IA64_CC_LDHINT);
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_FREG, bits, 13, 7);
            operand(i, 3, IA64_OPER_MEM, bits, 20, 7);
            i->i_srcidx++;
            break;
        case IA64_FMT_M12: /* 2 dst */
            ia64_hint(i, IA64_CC_LDHINT);
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_FREG, bits, 13, 7);
            operand(i, 3, IA64_OPER_MEM, bits, 20, 7);
            op_imm(i, 4, 8LL << FIELD(bits, 30, 1));
            i->i_srcidx++;
            break;
        case IA64_FMT_M13:
            ia64_hint(i, IA64_CC_LFHINT);
            operand(i, 1, IA64_OPER_MEM, bits, 20, 7);
            break;
        case IA64_FMT_M14: /* 0 dst */
            ia64_hint(i, IA64_CC_LFHINT);
            operand(i, 1, IA64_OPER_MEM, bits, 20, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            i->i_srcidx--;
            break;
        case IA64_FMT_M15: /* 0 dst */
            ia64_hint(i, IA64_CC_LFHINT);
            operand(i, 1, IA64_OPER_MEM, bits, 20, 7);
            s_immf(i, 2, bits, FRAG(13,7), FRAG(27,1), FRAG(36,1), 0);
            i->i_srcidx--;
            break;
        case IA64_FMT_M16:
            ia64_hint(i, IA64_CC_LDHINT);
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_MEM, bits, 20, 7);
            operand(i, 3, IA64_OPER_GREG, bits, 13, 7);
            if (i->i_op == IA64_OP_CMP8XCHG16) {
                op_type(i, 4, IA64_OPER_AREG);
                op_value(i, 4, AR_CSD);
                op_type(i, 5, IA64_OPER_AREG);
                op_value(i, 5, AR_CCV);
            } else {
                if (FIELD(bits, 30, 6) < 8) {
                    op_type(i, 4, IA64_OPER_AREG);
                    op_value(i, 4, AR_CCV);
                }
            }
            break;
        case IA64_FMT_M17:
            ia64_hint(i, IA64_CC_LDHINT);
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_MEM, bits, 20, 7);
            switch (FIELD(bits, 13, 2)) {
                case 0: op_imm(i, 3, 1LL << 4); break;
                case 1: op_imm(i, 3, 1LL << 3); break;
                case 2:	op_imm(i, 3, 1LL << 2); break;
                case 3: op_imm(i, 3, 1LL); break;
            }
            if (FIELD(bits, 15, 1))
                i->i_oper[3].o_value *= -1LL;
            break;
        case IA64_FMT_M18:
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            break;
        case IA64_FMT_M19:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_FREG, bits, 13, 7);
            break;
        case IA64_FMT_M20: /* 0 dst */
            operand(i, 1, IA64_OPER_GREG, bits, 13, 7);
            disp(i, 2, bits, FRAG(6,7), FRAG(20,13), FRAG(36,1), 0);
            i->i_srcidx--;
            break;
        case IA64_FMT_M21: /* 0 dst */
            operand(i, 1, IA64_OPER_FREG, bits, 13, 7);
            disp(i, 2, bits, FRAG(6,7), FRAG(20,13), FRAG(36,1), 0);
            i->i_srcidx--;
            break;
        case IA64_FMT_M22: /* 0 dst */
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            disp(i, 2, bits, FRAG(13,20), FRAG(36,1), 0);
            i->i_srcidx--;
            break;
        case IA64_FMT_M23: /* 0 dst */
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            disp(i, 2, bits, FRAG(13,20), FRAG(36,1), 0);
            i->i_srcidx--;
            break;
        case IA64_FMT_M24:
            /* no operands */
            break;
        case IA64_FMT_M25:
            if (FIELD(bits, 0, 6) != 0)
                return (0);
            /* no operands */
            break;
        case IA64_FMT_M26:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            break;
        case IA64_FMT_M27:
            operand(i, 1, IA64_OPER_FREG, bits, 6, 7);
            break;
        case IA64_FMT_M28:
            operand(i, 1, IA64_OPER_GREG, bits, 20, 7);
            break;
        case IA64_FMT_M29:
            operand(i, 1, IA64_OPER_AREG, bits, 20, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            break;
        case IA64_FMT_M30:
            operand(i, 1, IA64_OPER_AREG, bits, 20, 7);
            s_immf(i, 2, bits, FRAG(13,7), FRAG(36,1), 0);
            break;
        case IA64_FMT_M31:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_AREG, bits, 20, 7);
            break;
        case IA64_FMT_M32:
            operand(i, 1, IA64_OPER_CREG, bits, 20, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            break;
        case IA64_FMT_M33:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_CREG, bits, 20, 7);
            break;
        case IA64_FMT_M34: {
            uint64_t loc, out;
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            op_type(i, 2, IA64_OPER_AREG);
            op_value(i, 2, AR_PFS);
            loc = FIELD(bits, 20, 7);
            out = FIELD(bits, 13, 7) - loc;
            op_imm(i, 3, 0);
            op_imm(i, 4, loc);
            op_imm(i, 5, out);
            op_imm(i, 6, (uint64_t)FIELD(bits, 27, 4) << 3);
            break;
        }
        case IA64_FMT_M35:
            if (FIELD(bits, 27, 6) == 0x2D)
                op_type(i, 1, IA64_OPER_PSR_L);
            else
                op_type(i, 1, IA64_OPER_PSR_UM);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            break;
        case IA64_FMT_M36:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            if (FIELD(bits, 27, 6) == 0x25)
                op_type(i, 2, IA64_OPER_PSR);
            else
                op_type(i, 2, IA64_OPER_PSR_UM);
            break;
        case IA64_FMT_M37:
            u_immf(i, 1, bits, FRAG(6,20), FRAG(36,1), 0);
            break;
        case IA64_FMT_M38:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 20, 7);
            operand(i, 3, IA64_OPER_GREG, bits, 13, 7);
            break;
        case IA64_FMT_M39:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 20, 7);
            u_imm(i, 3, bits, 13, 2);
            break;
        case IA64_FMT_M40: /* 0 dst */
            operand(i, 1, IA64_OPER_GREG, bits, 20, 7);
            u_imm(i, 2, bits, 13, 2);
            i->i_srcidx--;
            break;
        case IA64_FMT_M41:
            operand(i, 1, IA64_OPER_GREG, bits, 13, 7);
            break;
        case IA64_FMT_M42:
            operand(i, 1, ot, bits, 20, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            break;
        case IA64_FMT_M43:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, ot, bits, 20, 7);
            break;
        case IA64_FMT_M44:
            u_immf(i, 1, bits, FRAG(6,21), FRAG(31,2), FRAG(36,1), 0);
            break;
        case IA64_FMT_M45: /* 0 dst */
            operand(i, 1, IA64_OPER_GREG, bits, 20, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 13, 7);
            i->i_srcidx--;
            break;
        case IA64_FMT_M46:
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            operand(i, 2, IA64_OPER_GREG, bits, 20, 7);
            break;
        case IA64_FMT_M47:
            operand(i, 1, IA64_OPER_GREG, bits, 20, 7);
            break;
        case IA64_FMT_M48:
            u_immf(i, 1, bits, FRAG(6,20), FRAG(36,1), 0);
            break;
        case IA64_FMT_X1:
            assert(slot == 2);
            u_immf(i, 1, bits, FRAG(6,20), FRAG(36,1), 0);
            combine(&i->i_oper[1].o_value, 21, b->b_inst[1].i_bits, 41, 0);
            break;
        case IA64_FMT_X2:
            assert(slot == 2);
            operand(i, 1, IA64_OPER_GREG, bits, 6, 7);
            u_immf(i, 2, bits, FRAG(13,7), FRAG(27,9), FRAG(22,5),
                   FRAG(21,1), 0);
            combine(&i->i_oper[2].o_value, 22, b->b_inst[1].i_bits, 41, 0);
            combine(&i->i_oper[2].o_value, 63, bits, 1, 36);
            break;
        case IA64_FMT_X3:
            assert(slot == 2);
            ia64_brhint(i);
            u_imm(i, 1, bits, 13, 20);
            combine(&i->i_oper[1].o_value, 20, b->b_inst[1].i_bits, 39, 2);
            combine(&i->i_oper[1].o_value, 59, bits, 1, 36);
            i->i_oper[1].o_value <<= 4;
            i->i_oper[1].o_type = IA64_OPER_DISP;
            break;
        case IA64_FMT_X4:
            assert(slot == 2);
            ia64_brhint(i);
            operand(i, 1, IA64_OPER_BREG, bits, 6, 3);
            u_imm(i, 2, bits, 13, 20);
            combine(&i->i_oper[2].o_value, 20, b->b_inst[1].i_bits, 39, 2);
            combine(&i->i_oper[2].o_value, 59, bits, 1, 36);
            i->i_oper[2].o_value <<= 4;
            i->i_oper[2].o_type = IA64_OPER_DISP;
            break;
        case IA64_FMT_X5:
            assert(slot == 2);
            u_immf(i, 1, bits, FRAG(6,20), FRAG(36,1), 0);
            combine(&i->i_oper[1].o_value, 21, b->b_inst[1].i_bits, 41, 0);
            break;
        default:
            assert(fmt == IA64_FMT_NONE);
            return (0);
	}
    
	return (1);
}

/*-
 * Copyright (c) 2000-2006 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//#include <sys/cdefs.h>

//#include <sys/endian.h>
#include <stdint.h>
#include <string.h>

#include "ia64_disas_int.h"
#include "ia64_disas.h"

/*
 * Template names.
 */
static const char *ia64_templname[] = {
	"MII", "MII;", "MI;I", "MI;I;", "MLX", "MLX;", 0, 0,
	"MMI", "MMI;", "M;MI", "M;MI;", "MFI", "MFI;", "MMF", "MMF;",
	"MIB", "MIB;", "MBB", "MBB;", 0, 0, "BBB", "BBB;",
	"MMB", "MMB;", 0, 0, "MFB", "MFB;", 0, 0
};

/*
 * Decode A-unit instructions.
 */
static int
ia64_decodeA(uint64_t bits, struct ia64_bundle *b, int slot)
{
	enum ia64_fmt fmt;
	enum ia64_op op;
    
	fmt = IA64_FMT_NONE, op = IA64_OP_NONE;
	switch((int)OPCODE(bits)) {
        case 0x8:
            switch (FIELD(bits, 34, 2)) { /* x2a */
                case 0x0:
                    if (FIELD(bits, 33, 1) == 0) { /* ve */
                        switch (FIELD(bits, 29, 4)) { /* x4 */
                            case 0x0:
                                if (FIELD(bits, 27, 2) <= 1) /* x2b */
                                    op = IA64_OP_ADD,
                                    fmt = IA64_FMT_A1;
                                break;
                            case 0x1:
                                if (FIELD(bits, 27, 2) <= 1) /* x2b */
                                    op = IA64_OP_SUB,
                                    fmt = IA64_FMT_A1;
                                break;
                            case 0x2:
                                if (FIELD(bits, 27, 2) == 0) /* x2b */
                                    op = IA64_OP_ADDP4,
                                    fmt = IA64_FMT_A1;
                                break;
                            case 0x3:
                                switch (FIELD(bits, 27, 2)) { /* x2b */
                                    case 0x0:
                                        op = IA64_OP_AND,
                                        fmt = IA64_FMT_A1;
                                        break;
                                    case 0x1:
                                        op = IA64_OP_ANDCM,
                                        fmt = IA64_FMT_A1;
                                        break;
                                    case 0x2:
                                        op = IA64_OP_OR,
                                        fmt = IA64_FMT_A1;
                                        break;
                                    case 0x3:
                                        op = IA64_OP_XOR,
                                        fmt = IA64_FMT_A1;
                                        break;
                                }
                                break;
                            case 0xB:
                                switch (FIELD(bits, 27, 2)) { /* x2b */
                                    case 0x0:
                                        op = IA64_OP_AND,
                                        fmt = IA64_FMT_A3;
                                        break;
                                    case 0x1:
                                        op = IA64_OP_ANDCM,
                                        fmt = IA64_FMT_A3;
                                        break;
                                    case 0x2:
                                        op = IA64_OP_OR,
                                        fmt = IA64_FMT_A3;
                                        break;
                                    case 0x3:
                                        op = IA64_OP_XOR,
                                        fmt = IA64_FMT_A3;
                                        break;
                                }
                                break;
                            case 0x4:
                                op = IA64_OP_SHLADD, fmt = IA64_FMT_A2;
                                break;
                            case 0x6:
                                op = IA64_OP_SHLADDP4,
                                fmt = IA64_FMT_A2;
                                break;
                            case 0x9:
                                if (FIELD(bits, 27, 2) == 1) /* x2b */
                                    op = IA64_OP_SUB,
                                    fmt = IA64_FMT_A3;
                                break;
                        }
                    }
                    break;
                case 0x1:
                    switch (FIELD(bits, 29, 8)) { /* za + x2a + zb + x4 */
                        case 0x20:
                            switch (FIELD(bits, 27, 2)) { /* x2b */
                                case 0x0:
                                    op = IA64_OP_PADD1_, fmt = IA64_FMT_A9;
                                    break;
                                case 0x1:
                                    op = IA64_OP_PADD1_SSS,
                                    fmt = IA64_FMT_A9;
                                    break;
                                case 0x2:
                                    op = IA64_OP_PADD1_UUU,
                                    fmt = IA64_FMT_A9;
                                    break;
                                case 0x3:
                                    op = IA64_OP_PADD1_UUS,
                                    fmt = IA64_FMT_A9;
                                    break;
                            }
                            break;
                        case 0x21:
                            switch (FIELD(bits, 27, 2)) { /* x2b */
                                case 0x0:
                                    op = IA64_OP_PSUB1_, fmt = IA64_FMT_A9;
                                    break;
                                case 0x1:
                                    op = IA64_OP_PSUB1_SSS,
                                    fmt = IA64_FMT_A9;
                                    break;
                                case 0x2:
                                    op = IA64_OP_PSUB1_UUU,
                                    fmt = IA64_FMT_A9;
                                    break;
                                case 0x3:
                                    op = IA64_OP_PSUB1_UUS,
                                    fmt = IA64_FMT_A9;
                                    break;
                            }
                            break;
                        case 0x22:
                            switch (FIELD(bits, 27, 2)) { /* x2b */
                                case 0x2:
                                    op = IA64_OP_PAVG1_, fmt = IA64_FMT_A9;
                                    break;
                                case 0x3:
                                    op = IA64_OP_PAVG1_RAZ,
                                    fmt = IA64_FMT_A9;
                                    break;
                            }
                            break;
                        case 0x23:
                            if (FIELD(bits, 27, 2) == 2) /* x2b */
                                op = IA64_OP_PAVGSUB1,
                                fmt = IA64_FMT_A9;
                            break;
                        case 0x29:
                            switch (FIELD(bits, 27, 2)) { /* x2b */
                                case 0x0:
                                    op = IA64_OP_PCMP1_EQ,
                                    fmt = IA64_FMT_A9;
                                    break;
                                case 0x1:
                                    op = IA64_OP_PCMP1_GT,
                                    fmt = IA64_FMT_A9;
                                    break;
                            }
                            break;
                        case 0x30:
                            switch (FIELD(bits, 27, 2)) { /* x2b */
                                case 0x0:
                                    op = IA64_OP_PADD2_, fmt = IA64_FMT_A9;
                                    break;
                                case 0x1:
                                    op = IA64_OP_PADD2_SSS,
                                    fmt = IA64_FMT_A9;
                                    break;
                                case 0x2:
                                    op = IA64_OP_PADD2_UUU,
                                    fmt = IA64_FMT_A9;
                                    break;
                                case 0x3:
                                    op = IA64_OP_PADD2_UUS,
                                    fmt = IA64_FMT_A9;
                                    break;
                            }
                            break;
                        case 0x31:
                            switch (FIELD(bits, 27, 2)) { /* x2b */
                                case 0x0:
                                    op = IA64_OP_PSUB2_, fmt = IA64_FMT_A9;
                                    break;
                                case 0x1:
                                    op = IA64_OP_PSUB2_SSS,
                                    fmt = IA64_FMT_A9;
                                    break;
                                case 0x2:
                                    op = IA64_OP_PSUB2_UUU,
                                    fmt = IA64_FMT_A9;
                                    break;
                                case 0x3:
                                    op = IA64_OP_PSUB2_UUS,
                                    fmt = IA64_FMT_A9;
                                    break;
                            }
                            break;
                        case 0x32:
                            switch (FIELD(bits, 27, 2)) { /* x2b */
                                case 0x2:
                                    op = IA64_OP_PAVG2_, fmt = IA64_FMT_A9;
                                    break;
                                case 0x3:
                                    op = IA64_OP_PAVG2_RAZ,
                                    fmt = IA64_FMT_A9;
                                    break;
                            }
                            break;
                        case 0x33:
                            if (FIELD(bits, 27, 2) == 2) /* x2b */
                                op = IA64_OP_PAVGSUB2,
                                fmt = IA64_FMT_A9;
                            break;
                        case 0x34:
                            op = IA64_OP_PSHLADD2, fmt = IA64_FMT_A10;
                            break;
                        case 0x36:
                            op = IA64_OP_PSHRADD2, fmt = IA64_FMT_A10;
                            break;
                        case 0x39:
                            switch (FIELD(bits, 27, 2)) { /* x2b */
                                case 0x0:
                                    op = IA64_OP_PCMP2_EQ,
                                    fmt = IA64_FMT_A9;
                                    break;
                                case 0x1:
                                    op = IA64_OP_PCMP2_GT,
                                    fmt = IA64_FMT_A9;
                                    break;
                            }
                            break;
                        case 0xA0:
                            if (FIELD(bits, 27, 2) == 0) /* x2b */
                                op = IA64_OP_PADD4, fmt = IA64_FMT_A9;
                            break;
                        case 0xA1:
                            if (FIELD(bits, 27, 2) == 0) /* x2b */
                                op = IA64_OP_PSUB4, fmt = IA64_FMT_A9;
                            break;
                        case 0xA9:
                            switch (FIELD(bits, 27, 2)) { /* x2b */
                                case 0x0:
                                    op = IA64_OP_PCMP4_EQ,
                                    fmt = IA64_FMT_A9;
                                    break;
                                case 0x1:
                                    op = IA64_OP_PCMP4_GT,
                                    fmt = IA64_FMT_A9;
                                    break;
                            }
                            break;
                    }
                    break;
                case 0x2:
                    if (FIELD(bits, 33, 1) == 0) /* ve */
                        op = IA64_OP_ADDS, fmt = IA64_FMT_A4;
                    break;
                case 0x3:
                    if (FIELD(bits, 33, 1) == 0) /* ve */
                        op = IA64_OP_ADDP4, fmt = IA64_FMT_A4;
                    break;
            }
            break;
        case 0x9:
            op = IA64_OP_ADDL, fmt = IA64_FMT_A5;
            break;
        case 0xC: case 0xD: case 0xE:
            if (FIELD(bits, 12, 1) == 0) { /* c */
                switch (FIELD(bits, 33, 8)) { /* maj + tb + x2 + ta */
                    case 0xC0:
                        op = IA64_OP_CMP_LT, fmt = IA64_FMT_A6;
                        break;
                    case 0xC1:
                        op = IA64_OP_CMP_EQ_AND, fmt = IA64_FMT_A6;
                        break;
                    case 0xC2:
                        op = IA64_OP_CMP4_LT, fmt = IA64_FMT_A6;
                        break;
                    case 0xC3:
                        op = IA64_OP_CMP4_EQ_AND, fmt = IA64_FMT_A6;
                        break;
                    case 0xC4: case 0xCC:
                        op = IA64_OP_CMP_LT, fmt = IA64_FMT_A8;
                        break;
                    case 0xC5: case 0xCD:
                        op = IA64_OP_CMP_EQ_AND, fmt = IA64_FMT_A8;
                        break;
                    case 0xC6: case 0xCE:
                        op = IA64_OP_CMP4_LT, fmt = IA64_FMT_A8;
                        break;
                    case 0xC7: case 0xCF:
                        op = IA64_OP_CMP4_EQ_AND, fmt = IA64_FMT_A8;
                        break;
                    case 0xC8:
                        op = IA64_OP_CMP_GT_AND, fmt = IA64_FMT_A7;
                        break;
                    case 0xC9:
                        op = IA64_OP_CMP_GE_AND, fmt = IA64_FMT_A7;
                        break;
                    case 0xCA:
                        op = IA64_OP_CMP4_GT_AND, fmt = IA64_FMT_A7;
                        break;
                    case 0xCB:
                        op = IA64_OP_CMP4_GE_AND, fmt = IA64_FMT_A7;
                        break;
                    case 0xD0:
                        op = IA64_OP_CMP_LTU, fmt = IA64_FMT_A6;
                        break;
                    case 0xD1:
                        op = IA64_OP_CMP_EQ_OR, fmt = IA64_FMT_A6;
                        break;
                    case 0xD2:
                        op = IA64_OP_CMP4_LTU, fmt = IA64_FMT_A6;
                        break;
                    case 0xD3:
                        op = IA64_OP_CMP4_EQ_OR, fmt = IA64_FMT_A6;
                        break;
                    case 0xD4: case 0xDC:
                        op = IA64_OP_CMP_LTU, fmt = IA64_FMT_A8;
                        break;
                    case 0xD5: case 0xDD:
                        op = IA64_OP_CMP_EQ_OR, fmt = IA64_FMT_A8;
                        break;
                    case 0xD6: case 0xDE:
                        op = IA64_OP_CMP4_LTU, fmt = IA64_FMT_A8;
                        break;
                    case 0xD7: case 0xDF:
                        op = IA64_OP_CMP4_EQ_OR, fmt = IA64_FMT_A8;
                        break;
                    case 0xD8:
                        op = IA64_OP_CMP_GT_OR, fmt = IA64_FMT_A7;
                        break;
                    case 0xD9:
                        op = IA64_OP_CMP_GE_OR, fmt = IA64_FMT_A7;
                        break;
                    case 0xDA:
                        op = IA64_OP_CMP4_GT_OR, fmt = IA64_FMT_A7;
                        break;
                    case 0xDB:
                        op = IA64_OP_CMP4_GE_OR, fmt = IA64_FMT_A7;
                        break;
                    case 0xE0:
                        op = IA64_OP_CMP_EQ, fmt = IA64_FMT_A6;
                        break;
                    case 0xE1:
                        op = IA64_OP_CMP_EQ_OR_ANDCM, fmt = IA64_FMT_A6;
                        break;
                    case 0xE2:
                        op = IA64_OP_CMP4_EQ, fmt = IA64_FMT_A6;
                        break;
                    case 0xE3:
                        op = IA64_OP_CMP4_EQ_OR_ANDCM,
                        fmt = IA64_FMT_A6;
                        break;
                    case 0xE4: case 0xEC:
                        op = IA64_OP_CMP_EQ, fmt = IA64_FMT_A8;
                        break;
                    case 0xE5: case 0xED:
                        op = IA64_OP_CMP_EQ_OR_ANDCM, fmt = IA64_FMT_A8;
                        break;
                    case 0xE6: case 0xEE:
                        op = IA64_OP_CMP4_EQ, fmt = IA64_FMT_A8;
                        break;
                    case 0xE7: case 0xEF:
                        op = IA64_OP_CMP4_EQ_OR_ANDCM,
                        fmt = IA64_FMT_A8;
                        break;
                    case 0xE8:
                        op = IA64_OP_CMP_GT_OR_ANDCM, fmt = IA64_FMT_A7;
                        break;
                    case 0xE9:
                        op = IA64_OP_CMP_GE_OR_ANDCM, fmt = IA64_FMT_A7;
                        break;
                    case 0xEA:
                        op = IA64_OP_CMP4_GT_OR_ANDCM,
                        fmt = IA64_FMT_A7;
                        break;
                    case 0xEB:
                        op = IA64_OP_CMP4_GE_OR_ANDCM,
                        fmt = IA64_FMT_A7;
                        break;
                }
            } else {
                switch (FIELD(bits, 33, 8)) { /* maj + tb + x2 + ta */
                    case 0xC0:
                        op = IA64_OP_CMP_LT_UNC, fmt = IA64_FMT_A6;
                        break;
                    case 0xC1:
                        op = IA64_OP_CMP_NE_AND, fmt = IA64_FMT_A6;
                        break;
                    case 0xC2:
                        op = IA64_OP_CMP4_LT_UNC, fmt = IA64_FMT_A6;
                        break;
                    case 0xC3:
                        op = IA64_OP_CMP4_NE_AND, fmt = IA64_FMT_A6;
                        break;
                    case 0xC4: case 0xCC:
                        op = IA64_OP_CMP_LT_UNC, fmt = IA64_FMT_A8;
                        break;
                    case 0xC5: case 0xCD:
                        op = IA64_OP_CMP_NE_AND, fmt = IA64_FMT_A8;
                        break;
                    case 0xC6: case 0xCE:
                        op = IA64_OP_CMP4_LT_UNC, fmt = IA64_FMT_A8;
                        break;
                    case 0xC7: case 0xCF:
                        op = IA64_OP_CMP4_NE_AND, fmt = IA64_FMT_A8;
                        break;
                    case 0xC8:
                        op = IA64_OP_CMP_LE_AND, fmt = IA64_FMT_A7;
                        break;
                    case 0xC9:
                        op = IA64_OP_CMP_LT_AND, fmt = IA64_FMT_A7;
                        break;
                    case 0xCA:
                        op = IA64_OP_CMP4_LE_AND, fmt = IA64_FMT_A7;
                        break;
                    case 0xCB:
                        op = IA64_OP_CMP4_LT_AND, fmt = IA64_FMT_A7;
                        break;
                    case 0xD0:
                        op = IA64_OP_CMP_LTU_UNC, fmt = IA64_FMT_A6;
                        break;
                    case 0xD1:
                        op = IA64_OP_CMP_NE_OR, fmt = IA64_FMT_A6;
                        break;
                    case 0xD2:
                        op = IA64_OP_CMP4_LTU_UNC, fmt = IA64_FMT_A6;
                        break;
                    case 0xD3:
                        op = IA64_OP_CMP4_NE_OR, fmt = IA64_FMT_A6;
                        break;
                    case 0xD4: case 0xDC:
                        op = IA64_OP_CMP_LTU_UNC, fmt = IA64_FMT_A8;
                        break;
                    case 0xD5: case 0xDD:
                        op = IA64_OP_CMP_NE_OR, fmt = IA64_FMT_A8;
                        break;
                    case 0xD6: case 0xDE:
                        op = IA64_OP_CMP4_LTU_UNC, fmt = IA64_FMT_A8;
                        break;
                    case 0xD7: case 0xDF:
                        op = IA64_OP_CMP4_NE_OR, fmt = IA64_FMT_A8;
                        break;
                    case 0xD8:
                        op = IA64_OP_CMP_LE_OR, fmt = IA64_FMT_A7;
                        break;
                    case 0xD9:
                        op = IA64_OP_CMP_LT_OR, fmt = IA64_FMT_A7;
                        break;
                    case 0xDA:
                        op = IA64_OP_CMP4_LE_OR, fmt = IA64_FMT_A7;
                        break;
                    case 0xDB:
                        op = IA64_OP_CMP4_LT_OR, fmt = IA64_FMT_A7;
                        break;
                    case 0xE0:
                        op = IA64_OP_CMP_EQ_UNC, fmt = IA64_FMT_A6;
                        break;
                    case 0xE1:
                        op = IA64_OP_CMP_NE_OR_ANDCM, fmt = IA64_FMT_A6;
                        break;
                    case 0xE2:
                        op = IA64_OP_CMP4_EQ_UNC, fmt = IA64_FMT_A6;
                        break;
                    case 0xE3:
                        op = IA64_OP_CMP4_NE_OR_ANDCM,
                        fmt = IA64_FMT_A6;
                        break;
                    case 0xE4: case 0xEC:
                        op = IA64_OP_CMP_EQ_UNC, fmt = IA64_FMT_A8;
                        break;
                    case 0xE5: case 0xED:
                        op = IA64_OP_CMP_NE_OR_ANDCM, fmt = IA64_FMT_A8;
                        break;
                    case 0xE6: case 0xEE:
                        op = IA64_OP_CMP4_EQ_UNC, fmt = IA64_FMT_A8;
                        break;
                    case 0xE7: case 0xEF:
                        op = IA64_OP_CMP4_NE_OR_ANDCM,
                        fmt = IA64_FMT_A8;
                        break;
                    case 0xE8:
                        op = IA64_OP_CMP_LE_OR_ANDCM, fmt = IA64_FMT_A7;
                        break;
                    case 0xE9:
                        op = IA64_OP_CMP_LT_OR_ANDCM, fmt = IA64_FMT_A7;
                        break;
                    case 0xEA:
                        op = IA64_OP_CMP4_LE_OR_ANDCM,
                        fmt = IA64_FMT_A7;
                        break;
                    case 0xEB:
                        op = IA64_OP_CMP4_LT_OR_ANDCM,
                        fmt = IA64_FMT_A7;
                        break;
                }
            }
            break;
	}
    
	if (op != IA64_OP_NONE)
		return (ia64_extract(op, fmt, bits, b, slot));
	return (0);
}

/*
 * Decode B-unit instructions.
 */
static int
ia64_decodeB(const void *ip, struct ia64_bundle *b, int slot)
{
	uint64_t bits;
	enum ia64_fmt fmt;
	enum ia64_op op;
    
	bits = SLOT(ip, slot);
	fmt = IA64_FMT_NONE, op = IA64_OP_NONE;
    
	switch((int)OPCODE(bits)) {
        case 0x0:
            switch (FIELD(bits, 27, 6)) { /* x6 */
                case 0x0:
                    op = IA64_OP_BREAK_B, fmt = IA64_FMT_B9;
                    break;
                case 0x2:
                    op = IA64_OP_COVER, fmt = IA64_FMT_B8;
                    break;
                case 0x4:
                    op = IA64_OP_CLRRRB_, fmt = IA64_FMT_B8;
                    break;
                case 0x5:
                    op = IA64_OP_CLRRRB_PR, fmt = IA64_FMT_B8;
                    break;
                case 0x8:
                    op = IA64_OP_RFI, fmt = IA64_FMT_B8;
                    break;
                case 0xC:
                    op = IA64_OP_BSW_0, fmt = IA64_FMT_B8;
                    break;
                case 0xD:
                    op = IA64_OP_BSW_1, fmt = IA64_FMT_B8;
                    break;
                case 0x10:
                    op = IA64_OP_EPC, fmt = IA64_FMT_B8;
                    break;
                case 0x18:
                    op = IA64_OP_VMSW_0, fmt = IA64_FMT_B8;
                    break;
                case 0x19:
                    op = IA64_OP_VMSW_1, fmt = IA64_FMT_B8;
                    break;
                case 0x20:
                    switch (FIELD(bits, 6, 3)) { /* btype */
                        case 0x0:
                            op = IA64_OP_BR_COND, fmt = IA64_FMT_B4;
                            break;
                        case 0x1:
                            op = IA64_OP_BR_IA, fmt = IA64_FMT_B4;
                            break;
                    }
                    break;
                case 0x21:
                    if (FIELD(bits, 6, 3) == 4) /* btype */
                        op = IA64_OP_BR_RET, fmt = IA64_FMT_B4;
                    break;
            }
            break;
        case 0x1:
            op = IA64_OP_BR_CALL, fmt = IA64_FMT_B5;
            break;
        case 0x2:
            switch (FIELD(bits, 27, 6)) { /* x6 */
                case 0x0:
                    op = IA64_OP_NOP_B, fmt = IA64_FMT_B9;
                    break;
                case 0x1:
                    op = IA64_OP_HINT_B, fmt = IA64_FMT_B9;
                    break;
                case 0x10:
                    op = IA64_OP_BRP_, fmt = IA64_FMT_B7;
                    break;
                case 0x11:
                    op = IA64_OP_BRP_RET, fmt = IA64_FMT_B7;
                    break;
            }
            break;
        case 0x4:
            switch (FIELD(bits, 6, 3)) { /* btype */
                case 0x0:
                    op = IA64_OP_BR_COND, fmt = IA64_FMT_B1;
                    break;
                case 0x2:
                    op = IA64_OP_BR_WEXIT, fmt = IA64_FMT_B1;
                    break;
                case 0x3:
                    op = IA64_OP_BR_WTOP, fmt = IA64_FMT_B1;
                    break;
                case 0x5:
                    op = IA64_OP_BR_CLOOP, fmt = IA64_FMT_B2;
                    break;
                case 0x6:
                    op = IA64_OP_BR_CEXIT, fmt = IA64_FMT_B2;
                    break;
                case 0x7:
                    op = IA64_OP_BR_CTOP, fmt = IA64_FMT_B2;
                    break;
            }
            break;
        case 0x5:
            op = IA64_OP_BR_CALL, fmt = IA64_FMT_B3;
            break;
        case 0x7:
            op = IA64_OP_BRP_, fmt = IA64_FMT_B6;
            break;
	}
    
	if (op != IA64_OP_NONE)
		return (ia64_extract(op, fmt, bits, b, slot));
	return (0);
}

/*
 * Decode F-unit instructions.
 */
static int
ia64_decodeF(const void *ip, struct ia64_bundle *b, int slot)
{
	uint64_t bits;
	enum ia64_fmt fmt;
	enum ia64_op op;
    
	bits = SLOT(ip, slot);
	fmt = IA64_FMT_NONE, op = IA64_OP_NONE;
    
	switch((int)OPCODE(bits)) {
        case 0x0:
            if (FIELD(bits, 33, 1) == 0) { /* x */
                switch (FIELD(bits, 27, 6)) { /* x6 */
                    case 0x0:
                        op = IA64_OP_BREAK_F, fmt = IA64_FMT_F15;
                        break;
                    case 0x1:
                        if (FIELD(bits, 26, 1) == 0) /* y */
                            op = IA64_OP_NOP_F, fmt = IA64_FMT_F16;
                        else  
                            op = IA64_OP_HINT_F, fmt = IA64_FMT_F16;
                        break;
                    case 0x4:
                        op = IA64_OP_FSETC, fmt = IA64_FMT_F12;
                        break;
                    case 0x5:
                        op = IA64_OP_FCLRF, fmt = IA64_FMT_F13;
                        break;
                    case 0x8:
                        op = IA64_OP_FCHKF, fmt = IA64_FMT_F14;
                        break;
                    case 0x10:
                        op = IA64_OP_FMERGE_S, fmt = IA64_FMT_F9;
                        break;
                    case 0x11:
                        op = IA64_OP_FMERGE_NS, fmt = IA64_FMT_F9;
                        break;
                    case 0x12:
                        op = IA64_OP_FMERGE_SE, fmt = IA64_FMT_F9;
                        break;
                    case 0x14:
                        op = IA64_OP_FMIN, fmt = IA64_FMT_F8;
                        break;
                    case 0x15:
                        op = IA64_OP_FMAX, fmt = IA64_FMT_F8;
                        break;
                    case 0x16:
                        op = IA64_OP_FAMIN, fmt = IA64_FMT_F8;
                        break;
                    case 0x17:
                        op = IA64_OP_FAMAX, fmt = IA64_FMT_F8;
                        break;
                    case 0x18:
                        op = IA64_OP_FCVT_FX, fmt = IA64_FMT_F10;
                        break;
                    case 0x19:
                        op = IA64_OP_FCVT_FXU, fmt = IA64_FMT_F10;
                        break;
                    case 0x1A:
                        op = IA64_OP_FCVT_FX_TRUNC, fmt = IA64_FMT_F10;
                        break;
                    case 0x1B:
                        op = IA64_OP_FCVT_FXU_TRUNC, fmt = IA64_FMT_F10;
                        break;
                    case 0x1C:
                        op = IA64_OP_FCVT_XF, fmt = IA64_FMT_F11;
                        break;
                    case 0x28:
                        op = IA64_OP_FPACK, fmt = IA64_FMT_F9;
                        break;
                    case 0x2C:
                        op = IA64_OP_FAND, fmt = IA64_FMT_F9;
                        break;
                    case 0x2D:
                        op = IA64_OP_FANDCM, fmt = IA64_FMT_F9;
                        break;
                    case 0x2E:
                        op = IA64_OP_FOR, fmt = IA64_FMT_F9;
                        break;
                    case 0x2F:
                        op = IA64_OP_FXOR, fmt = IA64_FMT_F9;
                        break;
                    case 0x34:
                        op = IA64_OP_FSWAP_, fmt = IA64_FMT_F9;
                        break;
                    case 0x35:
                        op = IA64_OP_FSWAP_NL, fmt = IA64_FMT_F9;
                        break;
                    case 0x36:
                        op = IA64_OP_FSWAP_NR, fmt = IA64_FMT_F9;
                        break;
                    case 0x39:
                        op = IA64_OP_FMIX_LR, fmt = IA64_FMT_F9;
                        break;
                    case 0x3A:
                        op = IA64_OP_FMIX_R, fmt = IA64_FMT_F9;
                        break;
                    case 0x3B:
                        op = IA64_OP_FMIX_L, fmt = IA64_FMT_F9;
                        break;
                    case 0x3C:
                        op = IA64_OP_FSXT_R, fmt = IA64_FMT_F9;
                        break;
                    case 0x3D:
                        op = IA64_OP_FSXT_L, fmt = IA64_FMT_F9;
                        break;
                }
            } else {
                if (FIELD(bits, 36, 1) == 0) /* q */
                    op = IA64_OP_FRCPA, fmt = IA64_FMT_F6;
                else
                    op = IA64_OP_FRSQRTA, fmt = IA64_FMT_F7;
            }
            break;
        case 0x1:
            if (FIELD(bits, 33, 1) == 0) { /* x */
                switch (FIELD(bits, 27, 6)) { /* x6 */
                    case 0x10:
                        op = IA64_OP_FPMERGE_S, fmt = IA64_FMT_F9;
                        break;
                    case 0x11:
                        op = IA64_OP_FPMERGE_NS, fmt = IA64_FMT_F9;
                        break;
                    case 0x12:
                        op = IA64_OP_FPMERGE_SE, fmt = IA64_FMT_F9;
                        break;
                    case 0x14:
                        op = IA64_OP_FPMIN, fmt = IA64_FMT_F8;
                        break;
                    case 0x15:
                        op = IA64_OP_FPMAX, fmt = IA64_FMT_F8;
                        break;
                    case 0x16:
                        op = IA64_OP_FPAMIN, fmt = IA64_FMT_F8;
                        break;
                    case 0x17:
                        op = IA64_OP_FPAMAX, fmt = IA64_FMT_F8;
                        break;
                    case 0x18:
                        op = IA64_OP_FPCVT_FX, fmt = IA64_FMT_F10;
                        break;
                    case 0x19:
                        op = IA64_OP_FPCVT_FXU, fmt = IA64_FMT_F10;
                        break;
                    case 0x1A:
                        op = IA64_OP_FPCVT_FX_TRUNC, fmt = IA64_FMT_F10;
                        break;
                    case 0x1B:
                        op = IA64_OP_FPCVT_FXU_TRUNC,
                        fmt = IA64_FMT_F10;
                        break;
                    case 0x30:
                        op = IA64_OP_FPCMP_EQ, fmt = IA64_FMT_F8;
                        break;
                    case 0x31:
                        op = IA64_OP_FPCMP_LT, fmt = IA64_FMT_F8;
                        break;
                    case 0x32:
                        op = IA64_OP_FPCMP_LE, fmt = IA64_FMT_F8;
                        break;
                    case 0x33:
                        op = IA64_OP_FPCMP_UNORD, fmt = IA64_FMT_F8;
                        break;
                    case 0x34:
                        op = IA64_OP_FPCMP_NEQ, fmt = IA64_FMT_F8;
                        break;
                    case 0x35:
                        op = IA64_OP_FPCMP_NLT, fmt = IA64_FMT_F8;
                        break;
                    case 0x36:
                        op = IA64_OP_FPCMP_NLE, fmt = IA64_FMT_F8;
                        break;
                    case 0x37:
                        op = IA64_OP_FPCMP_ORD, fmt = IA64_FMT_F8;
                        break;
                }
            } else {
                if (FIELD(bits, 36, 1) == 0) /* q */
                    op = IA64_OP_FPRCPA, fmt = IA64_FMT_F6;
                else
                    op = IA64_OP_FPRSQRTA, fmt = IA64_FMT_F7;
            }
            break;
        case 0x4:
            op = IA64_OP_FCMP, fmt = IA64_FMT_F4;
            break;
        case 0x5:
            op = IA64_OP_FCLASS_M, fmt = IA64_FMT_F5;
            break;
        case 0x8:
            if (FIELD(bits, 36, 1) == 0) /* x */
                op = IA64_OP_FMA_, fmt = IA64_FMT_F1;
            else
                op = IA64_OP_FMA_S, fmt = IA64_FMT_F1;
            break;
        case 0x9:
            if (FIELD(bits, 36, 1) == 0) /* x */
                op = IA64_OP_FMA_D, fmt = IA64_FMT_F1;
            else
                op = IA64_OP_FPMA, fmt = IA64_FMT_F1;
            break;
        case 0xA:
            if (FIELD(bits, 36, 1) == 0) /* x */
                op = IA64_OP_FMS_, fmt = IA64_FMT_F1;
            else
                op = IA64_OP_FMS_S, fmt = IA64_FMT_F1;
            break;
        case 0xB:
            if (FIELD(bits, 36, 1) == 0) /* x */
                op = IA64_OP_FMS_D, fmt = IA64_FMT_F1;
            else
                op = IA64_OP_FPMS, fmt = IA64_FMT_F1;
            break;
        case 0xC:
            if (FIELD(bits, 36, 1) == 0) /* x */
                op = IA64_OP_FNMA_, fmt = IA64_FMT_F1;
            else
                op = IA64_OP_FNMA_S, fmt = IA64_FMT_F1;
            break;
        case 0xD:
            if (FIELD(bits, 36, 1) == 0) /* x */
                op = IA64_OP_FNMA_D, fmt = IA64_FMT_F1;
            else
                op = IA64_OP_FPNMA, fmt = IA64_FMT_F1;
            break;
        case 0xE:
            if (FIELD(bits, 36, 1) == 1) { /* x */
                switch (FIELD(bits, 34, 2)) { /* x2 */
                    case 0x0:
                        op = IA64_OP_XMA_L, fmt = IA64_FMT_F2;
                        break;
                    case 0x2:
                        op = IA64_OP_XMA_HU, fmt = IA64_FMT_F2;
                        break;
                    case 0x3:
                        op = IA64_OP_XMA_H, fmt = IA64_FMT_F2;
                        break;
                }
            } else
                op = IA64_OP_FSELECT, fmt = IA64_FMT_F3;
            break;
	}
    
	if (op != IA64_OP_NONE)
		return (ia64_extract(op, fmt, bits, b, slot));
	return (0);
}

/*
 * Decode I-unit instructions.
 */
static int
ia64_decodeI(const void *ip, struct ia64_bundle *b, int slot)
{
	uint64_t bits;
	enum ia64_fmt fmt;
	enum ia64_op op;
    
	bits = SLOT(ip, slot);
	if ((int)OPCODE(bits) >= 8)
		return (ia64_decodeA(bits, b, slot));
	fmt = IA64_FMT_NONE, op = IA64_OP_NONE;
    
	switch((int)OPCODE(bits)) {
        case 0x0:
            switch (FIELD(bits, 33, 3)) { /* x3 */
                case 0x0:
                    switch (FIELD(bits, 27, 6)) { /* x6 */
                        case 0x0:
                            op = IA64_OP_BREAK_I, fmt = IA64_FMT_I19;
                            break;
                        case 0x1:
                            if (FIELD(bits, 26, 1) == 0) /* y */
                                op = IA64_OP_NOP_I, fmt = IA64_FMT_I18;
                            else
                                op = IA64_OP_HINT_I, fmt = IA64_FMT_I18;
                            break;
                        case 0xA:
                            op = IA64_OP_MOV_I, fmt = IA64_FMT_I27;
                            break;
                        case 0x10:
                            op = IA64_OP_ZXT1, fmt = IA64_FMT_I29;
                            break;
                        case 0x11:
                            op = IA64_OP_ZXT2, fmt = IA64_FMT_I29;
                            break;
                        case 0x12:
                            op = IA64_OP_ZXT4, fmt = IA64_FMT_I29;
                            break;
                        case 0x14:
                            op = IA64_OP_SXT1, fmt = IA64_FMT_I29;
                            break;
                        case 0x15:
                            op = IA64_OP_SXT2, fmt = IA64_FMT_I29;
                            break;
                        case 0x16:
                            op = IA64_OP_SXT4, fmt = IA64_FMT_I29;
                            break;
                        case 0x18:
                            op = IA64_OP_CZX1_L, fmt = IA64_FMT_I29;
                            break;
                        case 0x19:
                            op = IA64_OP_CZX2_L, fmt = IA64_FMT_I29;
                            break;
                        case 0x1C:
                            op = IA64_OP_CZX1_R, fmt = IA64_FMT_I29;
                            break;
                        case 0x1D:
                            op = IA64_OP_CZX2_R, fmt = IA64_FMT_I29;
                            break;
                        case 0x2A:
                            op = IA64_OP_MOV_I, fmt = IA64_FMT_I26;
                            break;
                        case 0x30:
                            op = IA64_OP_MOV_IP, fmt = IA64_FMT_I25;
                            break;
                        case 0x31:
                            op = IA64_OP_MOV_, fmt = IA64_FMT_I22;
                            break;
                        case 0x32:
                            op = IA64_OP_MOV_I, fmt = IA64_FMT_I28;
                            break;
                        case 0x33:
                            op = IA64_OP_MOV_PR, fmt = IA64_FMT_I25;
                            break;
                    }
                    break;
                case 0x1:
                    op = IA64_OP_CHK_S_I, fmt = IA64_FMT_I20;
                    break;
                case 0x2:
                    op = IA64_OP_MOV_, fmt = IA64_FMT_I24;
                    break;
                case 0x3:
                    op = IA64_OP_MOV_, fmt = IA64_FMT_I23;
                    break;
                case 0x7:
                    if (FIELD(bits, 22, 1) == 0) /* x */
                        op = IA64_OP_MOV_, fmt = IA64_FMT_I21;
                    else
                        op = IA64_OP_MOV_RET, fmt = IA64_FMT_I21;
                    break;
            }
            break;
        case 0x4:
            op = IA64_OP_DEP_, fmt = IA64_FMT_I15;
            break;
        case 0x5:
            switch (FIELD(bits, 33, 3)) { /* x + x2 */
                case 0x0:
                    if (FIELD(bits, 36, 1) == 0) { /* tb */
                        switch (FIELD(bits, 12, 2)) { /* c + y */
                            case 0x0:
                                op = IA64_OP_TBIT_Z, fmt = IA64_FMT_I16;
                                break;
                            case 0x1:
                                op = IA64_OP_TBIT_Z_UNC,
                                fmt = IA64_FMT_I16;
                                break;
                            case 0x2:
                                if (FIELD(bits, 19, 1) == 0) /* x */
                                    op = IA64_OP_TNAT_Z,
                                    fmt = IA64_FMT_I17;
                                else
                                    op = IA64_OP_TF_Z,
                                    fmt = IA64_FMT_I30;
                                break;
                            case 0x3:
                                if (FIELD(bits, 19, 1) == 0) /* x */
                                    op = IA64_OP_TNAT_Z_UNC,
                                    fmt = IA64_FMT_I17;
                                else
                                    op = IA64_OP_TF_Z_UNC,
                                    fmt = IA64_FMT_I30;
                                break;
                        }
                    } else {
                        switch (FIELD(bits, 12, 2)) { /* c + y */
                            case 0x0:
                                op = IA64_OP_TBIT_Z_AND,
                                fmt = IA64_FMT_I16;
                                break;
                            case 0x1:
                                op = IA64_OP_TBIT_NZ_AND,
                                fmt = IA64_FMT_I16;
                                break;
                            case 0x2:
                                if (FIELD(bits, 19, 1) == 0) /* x */
                                    op = IA64_OP_TNAT_Z_AND,
                                    fmt = IA64_FMT_I17;
                                else
                                    op = IA64_OP_TF_Z_AND,
                                    fmt = IA64_FMT_I30;
                                break;
                            case 0x3:
                                if (FIELD(bits, 19, 1) == 0) /* x */
                                    op = IA64_OP_TNAT_NZ_AND,
                                    fmt = IA64_FMT_I17;
                                else
                                    op = IA64_OP_TF_NZ_AND,
                                    fmt = IA64_FMT_I30;
                                break;
                        }
                    }
                    break;
                case 0x1:
                    if (FIELD(bits, 36, 1) == 0) { /* tb */
                        switch (FIELD(bits, 12, 2)) { /* c + y */
                            case 0x0:
                                op = IA64_OP_TBIT_Z_OR,
                                fmt = IA64_FMT_I16;
                                break;
                            case 0x1:
                                op = IA64_OP_TBIT_NZ_OR,
                                fmt = IA64_FMT_I16;
                                break;
                            case 0x2:
                                if (FIELD(bits, 19, 1) == 0) /* x */
                                    op = IA64_OP_TNAT_Z_OR,
                                    fmt = IA64_FMT_I17;
                                else
                                    op = IA64_OP_TF_Z_OR,
                                    fmt = IA64_FMT_I30;
                                break;
                            case 0x3:
                                if (FIELD(bits, 19, 1) == 0) /* x */
                                    op = IA64_OP_TNAT_NZ_OR,
                                    fmt = IA64_FMT_I17;
                                else
                                    op = IA64_OP_TF_NZ_OR,
                                    fmt = IA64_FMT_I30;
                                break;
                        }
                    } else {
                        switch (FIELD(bits, 12, 2)) { /* c + y */
                            case 0x0:
                                op = IA64_OP_TBIT_Z_OR_ANDCM,
                                fmt = IA64_FMT_I16;
                                break;
                            case 0x1:
                                op = IA64_OP_TBIT_NZ_OR_ANDCM,
                                fmt = IA64_FMT_I16;
                                break;
                            case 0x2:
                                if (FIELD(bits, 19, 1) == 0) /* x */
                                    op = IA64_OP_TNAT_Z_OR_ANDCM,
                                    fmt = IA64_FMT_I17;
                                else
                                    op = IA64_OP_TF_Z_OR_ANDCM,
                                    fmt = IA64_FMT_I30;
                                break;
                            case 0x3:
                                if (FIELD(bits, 19, 1) == 0) /* x */
                                    op = IA64_OP_TNAT_NZ_OR_ANDCM,
                                    fmt = IA64_FMT_I17;
                                else
                                    op = IA64_OP_TF_NZ_OR_ANDCM,
                                    fmt = IA64_FMT_I30;
                                break;
                        }
                    }
                    break;
                case 0x2:
                    op = IA64_OP_EXTR, fmt = IA64_FMT_I11;
                    break;
                case 0x3:
                    if (FIELD(bits, 26, 1) == 0) /* y */
                        op = IA64_OP_DEP_Z, fmt = IA64_FMT_I12;
                    else
                        op = IA64_OP_DEP_Z, fmt = IA64_FMT_I13;
                    break;
                case 0x6:
                    op = IA64_OP_SHRP, fmt = IA64_FMT_I10;
                    break;
                case 0x7:
                    op = IA64_OP_DEP_, fmt = IA64_FMT_I14;
                    break;
            }
            break;
        case 0x7:
            switch (FIELD(bits, 32, 5)) { /* ve + zb + x2a + za */
                case 0x2:
                    switch (FIELD(bits, 28, 4)) { /* x2b + x2c */
                        case 0x0:
                            op = IA64_OP_PSHR2_U, fmt = IA64_FMT_I5;
                            break;
                        case 0x1: case 0x5: case 0x9: case 0xD:
                            op = IA64_OP_PMPYSHR2_U, fmt = IA64_FMT_I1;
                            break;
                        case 0x2:
                            op = IA64_OP_PSHR2_, fmt = IA64_FMT_I5;
                            break;
                        case 0x3: case 0x7: case 0xB: case 0xF:
                            op = IA64_OP_PMPYSHR2_, fmt = IA64_FMT_I1;
                            break;
                        case 0x4:
                            op = IA64_OP_PSHL2, fmt = IA64_FMT_I7;
                            break;
                    }
                    break;
                case 0x6:
                    switch (FIELD(bits, 28, 4)) { /* x2b + x2c */
                        case 0x1:
                            op = IA64_OP_PSHR2_U, fmt = IA64_FMT_I6;
                            break;
                        case 0x3:
                            op = IA64_OP_PSHR2_, fmt = IA64_FMT_I6;
                            break;
                        case 0x9:
                            op = IA64_OP_POPCNT, fmt = IA64_FMT_I9;
                            break;
                    }
                    break;
                case 0x8:
                    switch (FIELD(bits, 28, 4)) { /* x2b + x2c */
                        case 0x1:
                            op = IA64_OP_PMIN1_U, fmt = IA64_FMT_I2;
                            break;
                        case 0x4:
                            op = IA64_OP_UNPACK1_H, fmt = IA64_FMT_I2;
                            break;
                        case 0x5:
                            op = IA64_OP_PMAX1_U, fmt = IA64_FMT_I2;
                            break;
                        case 0x6:
                            op = IA64_OP_UNPACK1_L, fmt = IA64_FMT_I2;
                            break;
                        case 0x8:
                            op = IA64_OP_MIX1_R, fmt = IA64_FMT_I2;
                            break;
                        case 0xA:
                            op = IA64_OP_MIX1_L, fmt = IA64_FMT_I2;
                            break;
                        case 0xB:
                            op = IA64_OP_PSAD1, fmt = IA64_FMT_I2;
                            break;
                    }
                    break;
                case 0xA:
                    switch (FIELD(bits, 28, 4)) { /* x2b + x2c */
                        case 0x0:
                            op = IA64_OP_PACK2_USS, fmt = IA64_FMT_I2;
                            break;
                        case 0x2:
                            op = IA64_OP_PACK2_SSS, fmt = IA64_FMT_I2;
                            break;
                        case 0x3:
                            op = IA64_OP_PMIN2, fmt = IA64_FMT_I2;
                            break;
                        case 0x4:
                            op = IA64_OP_UNPACK2_H, fmt = IA64_FMT_I2;
                            break;
                        case 0x6:
                            op = IA64_OP_UNPACK2_L, fmt = IA64_FMT_I2;
                            break;
                        case 0x7:
                            op = IA64_OP_PMAX2, fmt = IA64_FMT_I2;
                            break;
                        case 0x8:
                            op = IA64_OP_MIX2_R, fmt = IA64_FMT_I2;
                            break;
                        case 0xA:
                            op = IA64_OP_MIX2_L, fmt = IA64_FMT_I2;
                            break;
                        case 0xD:
                            op = IA64_OP_PMPY2_R, fmt = IA64_FMT_I2;
                            break;
                        case 0xF:
                            op = IA64_OP_PMPY2_L, fmt = IA64_FMT_I2;
                            break;
                    }
                    break;
                case 0xC:
                    switch (FIELD(bits, 28, 4)) { /* x2b + x2c */
                        case 0xA:
                            op = IA64_OP_MUX1, fmt = IA64_FMT_I3;
                            break;
                    }
                    break;
                case 0xE:
                    switch (FIELD(bits, 28, 4)) { /* x2b + x2c */
                        case 0x5:
                            op = IA64_OP_PSHL2, fmt = IA64_FMT_I8;
                            break;
                        case 0xA:
                            op = IA64_OP_MUX2, fmt = IA64_FMT_I4;
                            break;
                    }
                    break;
                case 0x10:
                    switch (FIELD(bits, 28, 4)) { /* x2b + x2c */
                        case 0x0:
                            op = IA64_OP_PSHR4_U, fmt = IA64_FMT_I5;
                            break;
                        case 0x2:
                            op = IA64_OP_PSHR4_, fmt = IA64_FMT_I5;
                            break;
                        case 0x4:
                            op = IA64_OP_PSHL4, fmt = IA64_FMT_I7;
                            break;
                    }
                    break;
                case 0x12:
                    switch (FIELD(bits, 28, 4)) { /* x2b + x2c */
                        case 0x0:
                            op = IA64_OP_SHR_U, fmt = IA64_FMT_I5;
                            break;
                        case 0x2:
                            op = IA64_OP_SHR_, fmt = IA64_FMT_I5;
                            break;
                        case 0x4:
                            op = IA64_OP_SHL, fmt = IA64_FMT_I7;
                            break;
                    }
                    break;
                case 0x14:
                    switch (FIELD(bits, 28, 4)) { /* x2b + x2c */
                        case 0x1:
                            op = IA64_OP_PSHR4_U, fmt = IA64_FMT_I6;
                            break;
                        case 0x3:
                            op = IA64_OP_PSHR4_, fmt = IA64_FMT_I6;
                            break;
                    }
                    break;
                case 0x18:
                    switch (FIELD(bits, 28, 4)) { /* x2b + x2c */
                        case 0x2:
                            op = IA64_OP_PACK4_SSS, fmt = IA64_FMT_I2;
                            break;
                        case 0x4:
                            op = IA64_OP_UNPACK4_H, fmt = IA64_FMT_I2;
                            break;
                        case 0x6:
                            op = IA64_OP_UNPACK4_L, fmt = IA64_FMT_I2;
                            break;
                        case 0x8:
                            op = IA64_OP_MIX4_R, fmt = IA64_FMT_I2;
                            break;
                        case 0xA:
                            op = IA64_OP_MIX4_L, fmt = IA64_FMT_I2;
                            break;
                    }
                    break;
                case 0x1C:
                    switch (FIELD(bits, 28, 4)) { /* x2b + x2c */
                        case 0x5:
                            op = IA64_OP_PSHL4, fmt = IA64_FMT_I8;
                            break;
                    }
                    break;
            }
            break;
	}
    
	if (op != IA64_OP_NONE)
		return (ia64_extract(op, fmt, bits, b, slot));
	return (0);
}

/*
 * Decode M-unit instructions.
 */
static int
ia64_decodeM(const void *ip, struct ia64_bundle *b, int slot)
{
	uint64_t bits;
	enum ia64_fmt fmt;
	enum ia64_op op;
    
	bits = SLOT(ip, slot);
	if ((int)OPCODE(bits) >= 8)
		return (ia64_decodeA(bits, b, slot));
	fmt = IA64_FMT_NONE, op = IA64_OP_NONE;
    
	switch((int)OPCODE(bits)) {
        case 0x0:
            switch (FIELD(bits, 33, 3)) { /* x3 */
                case 0x0:
                    switch (FIELD(bits, 27, 6)) { /* x6 (x4 + x2) */
                        case 0x0:
                            op = IA64_OP_BREAK_M, fmt = IA64_FMT_M37;
                            break;
                        case 0x1:
                            if (FIELD(bits, 26, 1) == 0) /* y */
                                op = IA64_OP_NOP_M, fmt = IA64_FMT_M48;
                            else
                                op = IA64_OP_HINT_M, fmt = IA64_FMT_M48;
                            break;
                        case 0x4: case 0x14: case 0x24: case 0x34:
                            op = IA64_OP_SUM, fmt = IA64_FMT_M44;
                            break;
                        case 0x5: case 0x15: case 0x25: case 0x35:
                            op = IA64_OP_RUM, fmt = IA64_FMT_M44;
                            break;
                        case 0x6: case 0x16: case 0x26: case 0x36:
                            op = IA64_OP_SSM, fmt = IA64_FMT_M44;
                            break;
                        case 0x7: case 0x17: case 0x27: case 0x37:
                            op = IA64_OP_RSM, fmt = IA64_FMT_M44;
                            break;
                        case 0xA:
                            op = IA64_OP_LOADRS, fmt = IA64_FMT_M25;
                            break;
                        case 0xC:
                            op = IA64_OP_FLUSHRS, fmt = IA64_FMT_M25;
                            break;
                        case 0x10:
                            op = IA64_OP_INVALA_, fmt = IA64_FMT_M24;
                            break;
                        case 0x12:
                            op = IA64_OP_INVALA_E, fmt = IA64_FMT_M26;
                            break;
                        case 0x13:
                            op = IA64_OP_INVALA_E, fmt = IA64_FMT_M27;
                            break;
                        case 0x20:
                            op = IA64_OP_FWB, fmt = IA64_FMT_M24;
                            break;
                        case 0x22:
                            op = IA64_OP_MF_, fmt = IA64_FMT_M24;
                            break;
                        case 0x23:
                            op = IA64_OP_MF_A, fmt = IA64_FMT_M24;
                            break;
                        case 0x28:
                            op = IA64_OP_MOV_M, fmt = IA64_FMT_M30;
                            break;
                        case 0x30:
                            op = IA64_OP_SRLZ_D, fmt = IA64_FMT_M24;
                            break;
                        case 0x31:
                            op = IA64_OP_SRLZ_I, fmt = IA64_FMT_M24;
                            break;
                        case 0x33:
                            op = IA64_OP_SYNC_I, fmt = IA64_FMT_M24;
                            break;
                    }
                    break;
                case 0x4:
                    op = IA64_OP_CHK_A_NC, fmt = IA64_FMT_M22;
                    break;
                case 0x5:
                    op = IA64_OP_CHK_A_CLR, fmt = IA64_FMT_M22;
                    break;
                case 0x6:
                    op = IA64_OP_CHK_A_NC, fmt = IA64_FMT_M23;
                    break;
                case 0x7:
                    op = IA64_OP_CHK_A_CLR, fmt = IA64_FMT_M23;
                    break;
            }
            break;
        case 0x1:
            switch (FIELD(bits, 33, 3)) { /* x3 */
                case 0x0:
                    switch (FIELD(bits, 27, 6)) { /* x6 (x4 + x2) */
                        case 0x0:
                            op = IA64_OP_MOV_RR, fmt = IA64_FMT_M42;
                            break;
                        case 0x1:
                            op = IA64_OP_MOV_DBR, fmt = IA64_FMT_M42;
                            break;
                        case 0x2:
                            op = IA64_OP_MOV_IBR, fmt = IA64_FMT_M42;
                            break;
                        case 0x3:
                            op = IA64_OP_MOV_PKR, fmt = IA64_FMT_M42;
                            break;
                        case 0x4:
                            op = IA64_OP_MOV_PMC, fmt = IA64_FMT_M42;
                            break;
                        case 0x5:
                            op = IA64_OP_MOV_PMD, fmt = IA64_FMT_M42;
                            break;
                        case 0x6:
                            op = IA64_OP_MOV_MSR, fmt = IA64_FMT_M42;
                            break;
                        case 0x9:
                            op = IA64_OP_PTC_L, fmt = IA64_FMT_M45;
                            break;
                        case 0xA:
                            op = IA64_OP_PTC_G, fmt = IA64_FMT_M45;
                            break;
                        case 0xB:
                            op = IA64_OP_PTC_GA, fmt = IA64_FMT_M45;
                            break;
                        case 0xC:
                            op = IA64_OP_PTR_D, fmt = IA64_FMT_M45;
                            break;
                        case 0xD:
                            op = IA64_OP_PTR_I, fmt = IA64_FMT_M45;
                            break;
                        case 0xE:
                            op = IA64_OP_ITR_D, fmt = IA64_FMT_M42;
                            break;
                        case 0xF:
                            op = IA64_OP_ITR_I, fmt = IA64_FMT_M42;
                            break;
                        case 0x10:
                            op = IA64_OP_MOV_RR, fmt = IA64_FMT_M43;
                            break;
                        case 0x11:
                            op = IA64_OP_MOV_DBR, fmt = IA64_FMT_M43;
                            break;
                        case 0x12:
                            op = IA64_OP_MOV_IBR, fmt = IA64_FMT_M43;
                            break;
                        case 0x13:
                            op = IA64_OP_MOV_PKR, fmt = IA64_FMT_M43;
                            break;
                        case 0x14:
                            op = IA64_OP_MOV_PMC, fmt = IA64_FMT_M43;
                            break;
                        case 0x15:
                            op = IA64_OP_MOV_PMD, fmt = IA64_FMT_M43;
                            break;
                        case 0x16:
                            op = IA64_OP_MOV_MSR, fmt = IA64_FMT_M43;
                            break;
                        case 0x17:
                            op = IA64_OP_MOV_CPUID, fmt = IA64_FMT_M43;
                            break;
                        case 0x18:
                            op = IA64_OP_PROBE_R, fmt = IA64_FMT_M39;
                            break;
                        case 0x19:
                            op = IA64_OP_PROBE_W, fmt = IA64_FMT_M39;
                            break;
                        case 0x1A:
                            op = IA64_OP_THASH, fmt = IA64_FMT_M46;
                            break;
                        case 0x1B:
                            op = IA64_OP_TTAG, fmt = IA64_FMT_M46;
                            break;
                        case 0x1E:
                            op = IA64_OP_TPA, fmt = IA64_FMT_M46;
                            break;
                        case 0x1F:
                            op = IA64_OP_TAK, fmt = IA64_FMT_M46;
                            break;
                        case 0x21:
                            op = IA64_OP_MOV_PSR_UM, fmt = IA64_FMT_M36;
                            break;
                        case 0x22:
                            op = IA64_OP_MOV_M, fmt = IA64_FMT_M31;
                            break;
                        case 0x24:
                            op = IA64_OP_MOV_, fmt = IA64_FMT_M33;
                            break;
                        case 0x25:
                            op = IA64_OP_MOV_PSR, fmt = IA64_FMT_M36;
                            break;
                        case 0x29:
                            op = IA64_OP_MOV_PSR_UM, fmt = IA64_FMT_M35;
                            break;
                        case 0x2A:
                            op = IA64_OP_MOV_M, fmt = IA64_FMT_M29;
                            break;
                        case 0x2C:
                            op = IA64_OP_MOV_, fmt = IA64_FMT_M32;
                            break;
                        case 0x2D:
                            op = IA64_OP_MOV_PSR_L, fmt = IA64_FMT_M35;
                            break;
                        case 0x2E:
                            op = IA64_OP_ITC_D, fmt = IA64_FMT_M41;
                            break;
                        case 0x2F:
                            op = IA64_OP_ITC_I, fmt = IA64_FMT_M41;
                            break;
                        case 0x30:
                            if (FIELD(bits, 36, 1) == 0) /* x */
                                op = IA64_OP_FC_, fmt = IA64_FMT_M28;
                            else
                                op = IA64_OP_FC_I, fmt = IA64_FMT_M28;
                            break;
                        case 0x31:
                            op = IA64_OP_PROBE_RW_FAULT, fmt = IA64_FMT_M40;
                            break;
                        case 0x32:
                            op = IA64_OP_PROBE_R_FAULT, fmt = IA64_FMT_M40;
                            break;
                        case 0x33:
                            op = IA64_OP_PROBE_W_FAULT, fmt = IA64_FMT_M40;
                            break;
                        case 0x34:
                            op = IA64_OP_PTC_E, fmt = IA64_FMT_M47;
                            break;
                        case 0x38:
                            op = IA64_OP_PROBE_R, fmt = IA64_FMT_M38;
                            break;
                        case 0x39:
                            op = IA64_OP_PROBE_W, fmt = IA64_FMT_M38;
                            break;
                    }
                    break;
                case 0x1:
                    op = IA64_OP_CHK_S_M, fmt = IA64_FMT_M20;
                    break;
                case 0x3:
                    op = IA64_OP_CHK_S, fmt = IA64_FMT_M21;
                    break;
                case 0x6:
                    op = IA64_OP_ALLOC, fmt = IA64_FMT_M34;
                    break;
            }
            break;
        case 0x4:
            if (FIELD(bits, 27, 1) == 0) { /* x */
                switch (FIELD(bits, 30, 7)) { /* x6 + m */
                    case 0x0:
                        op = IA64_OP_LD1_, fmt = IA64_FMT_M1;
                        break;
                    case 0x1:
                        op = IA64_OP_LD2_, fmt = IA64_FMT_M1;
                        break;
                    case 0x2:
                        op = IA64_OP_LD4_, fmt = IA64_FMT_M1;
                        break;
                    case 0x3:
                        op = IA64_OP_LD8_, fmt = IA64_FMT_M1;
                        break;
                    case 0x4:
                        op = IA64_OP_LD1_S, fmt = IA64_FMT_M1;
                        break;
                    case 0x5:
                        op = IA64_OP_LD2_S, fmt = IA64_FMT_M1;
                        break;
                    case 0x6:
                        op = IA64_OP_LD4_S, fmt = IA64_FMT_M1;
                        break;
                    case 0x7:
                        op = IA64_OP_LD8_S, fmt = IA64_FMT_M1;
                        break;
                    case 0x8:
                        op = IA64_OP_LD1_A, fmt = IA64_FMT_M1;
                        break;
                    case 0x9:
                        op = IA64_OP_LD2_A, fmt = IA64_FMT_M1;
                        break;
                    case 0xA:
                        op = IA64_OP_LD4_A, fmt = IA64_FMT_M1;
                        break;
                    case 0xB:
                        op = IA64_OP_LD8_A, fmt = IA64_FMT_M1;
                        break;
                    case 0xC:
                        op = IA64_OP_LD1_SA, fmt = IA64_FMT_M1;
                        break;
                    case 0xD:
                        op = IA64_OP_LD2_SA, fmt = IA64_FMT_M1;
                        break;
                    case 0xE:
                        op = IA64_OP_LD4_SA, fmt = IA64_FMT_M1;
                        break;
                    case 0xF:
                        op = IA64_OP_LD8_SA, fmt = IA64_FMT_M1;
                        break;
                    case 0x10:
                        op = IA64_OP_LD1_BIAS, fmt = IA64_FMT_M1;
                        break;
                    case 0x11:
                        op = IA64_OP_LD2_BIAS, fmt = IA64_FMT_M1;
                        break;
                    case 0x12:
                        op = IA64_OP_LD4_BIAS, fmt = IA64_FMT_M1;
                        break;
                    case 0x13:
                        op = IA64_OP_LD8_BIAS, fmt = IA64_FMT_M1;
                        break;
                    case 0x14:
                        op = IA64_OP_LD1_ACQ, fmt = IA64_FMT_M1;
                        break;
                    case 0x15:
                        op = IA64_OP_LD2_ACQ, fmt = IA64_FMT_M1;
                        break;
                    case 0x16:
                        op = IA64_OP_LD4_ACQ, fmt = IA64_FMT_M1;
                        break;
                    case 0x17:
                        op = IA64_OP_LD8_ACQ, fmt = IA64_FMT_M1;
                        break;
                    case 0x1B:
                        op = IA64_OP_LD8_FILL, fmt = IA64_FMT_M1;
                        break;
                    case 0x20:
                        op = IA64_OP_LD1_C_CLR, fmt = IA64_FMT_M1;
                        break;
                    case 0x21:
                        op = IA64_OP_LD2_C_CLR, fmt = IA64_FMT_M1;
                        break;
                    case 0x22:
                        op = IA64_OP_LD4_C_CLR, fmt = IA64_FMT_M1;
                        break;
                    case 0x23:
                        op = IA64_OP_LD8_C_CLR, fmt = IA64_FMT_M1;
                        break;
                    case 0x24:
                        op = IA64_OP_LD1_C_NC, fmt = IA64_FMT_M1;
                        break;
                    case 0x25:
                        op = IA64_OP_LD2_C_NC, fmt = IA64_FMT_M1;
                        break;
                    case 0x26:
                        op = IA64_OP_LD4_C_NC, fmt = IA64_FMT_M1;
                        break;
                    case 0x27:
                        op = IA64_OP_LD8_C_NC, fmt = IA64_FMT_M1;
                        break;
                    case 0x28:
                        op = IA64_OP_LD1_C_CLR_ACQ, fmt = IA64_FMT_M1;
                        break;
                    case 0x29:
                        op = IA64_OP_LD2_C_CLR_ACQ, fmt = IA64_FMT_M1;
                        break;
                    case 0x2A:
                        op = IA64_OP_LD4_C_CLR_ACQ, fmt = IA64_FMT_M1;
                        break;
                    case 0x2B:
                        op = IA64_OP_LD8_C_CLR_ACQ, fmt = IA64_FMT_M1;
                        break;
                    case 0x30:
                        op = IA64_OP_ST1_, fmt = IA64_FMT_M4;
                        break;
                    case 0x31:
                        op = IA64_OP_ST2_, fmt = IA64_FMT_M4;
                        break;
                    case 0x32:
                        op = IA64_OP_ST4_, fmt = IA64_FMT_M4;
                        break;
                    case 0x33:
                        op = IA64_OP_ST8_, fmt = IA64_FMT_M4;
                        break;
                    case 0x34:
                        op = IA64_OP_ST1_REL, fmt = IA64_FMT_M4;
                        break;
                    case 0x35:
                        op = IA64_OP_ST2_REL, fmt = IA64_FMT_M4;
                        break;
                    case 0x36:
                        op = IA64_OP_ST4_REL, fmt = IA64_FMT_M4;
                        break;
                    case 0x37:
                        op = IA64_OP_ST8_REL, fmt = IA64_FMT_M4;
                        break;
                    case 0x3B:
                        op = IA64_OP_ST8_SPILL, fmt = IA64_FMT_M4;
                        break;
                    case 0x40:
                        op = IA64_OP_LD1_, fmt = IA64_FMT_M2;
                        break;
                    case 0x41:
                        op = IA64_OP_LD2_, fmt = IA64_FMT_M2;
                        break;
                    case 0x42:
                        op = IA64_OP_LD4_, fmt = IA64_FMT_M2;
                        break;
                    case 0x43:
                        op = IA64_OP_LD8_, fmt = IA64_FMT_M2;
                        break;
                    case 0x44:
                        op = IA64_OP_LD1_S, fmt = IA64_FMT_M2;
                        break;
                    case 0x45:
                        op = IA64_OP_LD2_S, fmt = IA64_FMT_M2;
                        break;
                    case 0x46:
                        op = IA64_OP_LD4_S, fmt = IA64_FMT_M2;
                        break;
                    case 0x47:
                        op = IA64_OP_LD8_S, fmt = IA64_FMT_M2;
                        break;
                    case 0x48:
                        op = IA64_OP_LD1_A, fmt = IA64_FMT_M2;
                        break;
                    case 0x49:
                        op = IA64_OP_LD2_A, fmt = IA64_FMT_M2;
                        break;
                    case 0x4A:
                        op = IA64_OP_LD4_A, fmt = IA64_FMT_M2;
                        break;
                    case 0x4B:
                        op = IA64_OP_LD8_A, fmt = IA64_FMT_M2;
                        break;
                    case 0x4C:
                        op = IA64_OP_LD1_SA, fmt = IA64_FMT_M2;
                        break;
                    case 0x4D:
                        op = IA64_OP_LD2_SA, fmt = IA64_FMT_M2;
                        break;
                    case 0x4E:
                        op = IA64_OP_LD4_SA, fmt = IA64_FMT_M2;
                        break;
                    case 0x4F:
                        op = IA64_OP_LD8_SA, fmt = IA64_FMT_M2;
                        break;
                    case 0x50:
                        op = IA64_OP_LD1_BIAS, fmt = IA64_FMT_M2;
                        break;
                    case 0x51:
                        op = IA64_OP_LD2_BIAS, fmt = IA64_FMT_M2;
                        break;
                    case 0x52:
                        op = IA64_OP_LD4_BIAS, fmt = IA64_FMT_M2;
                        break;
                    case 0x53:
                        op = IA64_OP_LD8_BIAS, fmt = IA64_FMT_M2;
                        break;
                    case 0x54:
                        op = IA64_OP_LD1_ACQ, fmt = IA64_FMT_M2;
                        break;
                    case 0x55:
                        op = IA64_OP_LD2_ACQ, fmt = IA64_FMT_M2;
                        break;
                    case 0x56:
                        op = IA64_OP_LD4_ACQ, fmt = IA64_FMT_M2;
                        break;
                    case 0x57:
                        op = IA64_OP_LD8_ACQ, fmt = IA64_FMT_M2;
                        break;
                    case 0x5B:
                        op = IA64_OP_LD8_FILL, fmt = IA64_FMT_M2;
                        break;
                    case 0x60:
                        op = IA64_OP_LD1_C_CLR, fmt = IA64_FMT_M2;
                        break;
                    case 0x61:
                        op = IA64_OP_LD2_C_CLR, fmt = IA64_FMT_M2;
                        break;
                    case 0x62:
                        op = IA64_OP_LD4_C_CLR, fmt = IA64_FMT_M2;
                        break;
                    case 0x63:
                        op = IA64_OP_LD8_C_CLR, fmt = IA64_FMT_M2;
                        break;
                    case 0x64:
                        op = IA64_OP_LD1_C_NC, fmt = IA64_FMT_M2;
                        break;
                    case 0x65:
                        op = IA64_OP_LD2_C_NC, fmt = IA64_FMT_M2;
                        break;
                    case 0x66:
                        op = IA64_OP_LD4_C_NC, fmt = IA64_FMT_M2;
                        break;
                    case 0x67:
                        op = IA64_OP_LD8_C_NC, fmt = IA64_FMT_M2;
                        break;
                    case 0x68:
                        op = IA64_OP_LD1_C_CLR_ACQ, fmt = IA64_FMT_M2;
                        break;
                    case 0x69:
                        op = IA64_OP_LD2_C_CLR_ACQ, fmt = IA64_FMT_M2;
                        break;
                    case 0x6A:
                        op = IA64_OP_LD4_C_CLR_ACQ, fmt = IA64_FMT_M2;
                        break;
                    case 0x6B:
                        op = IA64_OP_LD8_C_CLR_ACQ, fmt = IA64_FMT_M2;
                        break;
                }
            } else {
                switch (FIELD(bits, 30, 7)) { /* x6 + m */
                    case 0x0:
                        op = IA64_OP_CMPXCHG1_ACQ, fmt = IA64_FMT_M16;
                        break;
                    case 0x1:
                        op = IA64_OP_CMPXCHG2_ACQ, fmt = IA64_FMT_M16;
                        break;
                    case 0x2:
                        op = IA64_OP_CMPXCHG4_ACQ, fmt = IA64_FMT_M16;
                        break;
                    case 0x3:
                        op = IA64_OP_CMPXCHG8_ACQ, fmt = IA64_FMT_M16;
                        break;
                    case 0x4:
                        op = IA64_OP_CMPXCHG1_REL, fmt = IA64_FMT_M16;
                        break;
                    case 0x5:
                        op = IA64_OP_CMPXCHG2_REL, fmt = IA64_FMT_M16;
                        break;
                    case 0x6:
                        op = IA64_OP_CMPXCHG4_REL, fmt = IA64_FMT_M16;
                        break;
                    case 0x7:
                        op = IA64_OP_CMPXCHG8_REL, fmt = IA64_FMT_M16;
                        break;
                    case 0x8:
                        op = IA64_OP_XCHG1, fmt = IA64_FMT_M16;
                        break;
                    case 0x9:
                        op = IA64_OP_XCHG2, fmt = IA64_FMT_M16;
                        break;
                    case 0xA:
                        op = IA64_OP_XCHG4, fmt = IA64_FMT_M16;
                        break;
                    case 0xB:
                        op = IA64_OP_XCHG8, fmt = IA64_FMT_M16;
                        break;
                    case 0x12:
                        op = IA64_OP_FETCHADD4_ACQ, fmt = IA64_FMT_M17;
                        break;
                    case 0x13:
                        op = IA64_OP_FETCHADD8_ACQ, fmt = IA64_FMT_M17;
                        break;
                    case 0x16:
                        op = IA64_OP_FETCHADD4_REL, fmt = IA64_FMT_M17;
                        break;
                    case 0x17:
                        op = IA64_OP_FETCHADD8_REL, fmt = IA64_FMT_M17;
                        break;
                    case 0x1C:
                        op = IA64_OP_GETF_SIG, fmt = IA64_FMT_M19;
                        break;
                    case 0x1D:
                        op = IA64_OP_GETF_EXP, fmt = IA64_FMT_M19;
                        break;
                    case 0x1E:
                        op = IA64_OP_GETF_S, fmt = IA64_FMT_M19;
                        break;
                    case 0x1F:
                        op = IA64_OP_GETF_D, fmt = IA64_FMT_M19;
                        break;
                    case 0x20:
                        op = IA64_OP_CMP8XCHG16_ACQ, fmt = IA64_FMT_M16;
                        break;
                    case 0x24:
                        op = IA64_OP_CMP8XCHG16_REL, fmt = IA64_FMT_M16;
                        break;
                    case 0x28:
                        op = IA64_OP_LD16_, fmt = IA64_FMT_M1;
                        break;
                    case 0x2C:
                        op = IA64_OP_LD16_ACQ, fmt = IA64_FMT_M1;
                        break;
                    case 0x30:
                        op = IA64_OP_ST16_, fmt = IA64_FMT_M4;
                        break;
                    case 0x34:
                        op = IA64_OP_ST16_REL, fmt = IA64_FMT_M4;
                        break;
                }
            }
            break;
        case 0x5:
            switch (FIELD(bits, 30, 6)) { /* x6 */
                case 0x0:
                    op = IA64_OP_LD1_, fmt = IA64_FMT_M3;
                    break;
                case 0x1:
                    op = IA64_OP_LD2_, fmt = IA64_FMT_M3;
                    break;
                case 0x2:
                    op = IA64_OP_LD4_, fmt = IA64_FMT_M3;
                    break;
                case 0x3:
                    op = IA64_OP_LD8_, fmt = IA64_FMT_M3;
                    break;
                case 0x4:
                    op = IA64_OP_LD1_S, fmt = IA64_FMT_M3;
                    break;
                case 0x5:
                    op = IA64_OP_LD2_S, fmt = IA64_FMT_M3;
                    break;
                case 0x6:
                    op = IA64_OP_LD4_S, fmt = IA64_FMT_M3;
                    break;
                case 0x7:
                    op = IA64_OP_LD8_S, fmt = IA64_FMT_M3;
                    break;
                case 0x8:
                    op = IA64_OP_LD1_A, fmt = IA64_FMT_M3;
                    break;
                case 0x9:
                    op = IA64_OP_LD2_A, fmt = IA64_FMT_M3;
                    break;
                case 0xA:
                    op = IA64_OP_LD4_A, fmt = IA64_FMT_M3;
                    break;
                case 0xB:
                    op = IA64_OP_LD8_A, fmt = IA64_FMT_M3;
                    break;
                case 0xC:
                    op = IA64_OP_LD1_SA, fmt = IA64_FMT_M3;
                    break;
                case 0xD:
                    op = IA64_OP_LD2_SA, fmt = IA64_FMT_M3;
                    break;
                case 0xE:
                    op = IA64_OP_LD4_SA, fmt = IA64_FMT_M3;
                    break;
                case 0xF:
                    op = IA64_OP_LD8_SA, fmt = IA64_FMT_M3;
                    break;
                case 0x10:
                    op = IA64_OP_LD1_BIAS, fmt = IA64_FMT_M3;
                    break;
                case 0x11:
                    op = IA64_OP_LD2_BIAS, fmt = IA64_FMT_M3;
                    break;
                case 0x12:
                    op = IA64_OP_LD4_BIAS, fmt = IA64_FMT_M3;
                    break;
                case 0x13:
                    op = IA64_OP_LD8_BIAS, fmt = IA64_FMT_M3;
                    break;
                case 0x14:
                    op = IA64_OP_LD1_ACQ, fmt = IA64_FMT_M3;
                    break;
                case 0x15:
                    op = IA64_OP_LD2_ACQ, fmt = IA64_FMT_M3;
                    break;
                case 0x16:
                    op = IA64_OP_LD4_ACQ, fmt = IA64_FMT_M3;
                    break;
                case 0x17:
                    op = IA64_OP_LD8_ACQ, fmt = IA64_FMT_M3;
                    break;
                case 0x1B:
                    op = IA64_OP_LD8_FILL, fmt = IA64_FMT_M3;
                    break;
                case 0x20:
                    op = IA64_OP_LD1_C_CLR, fmt = IA64_FMT_M3;
                    break;
                case 0x21:
                    op = IA64_OP_LD2_C_CLR, fmt = IA64_FMT_M3;
                    break;
                case 0x22:
                    op = IA64_OP_LD4_C_CLR, fmt = IA64_FMT_M3;
                    break;
                case 0x23:
                    op = IA64_OP_LD8_C_CLR, fmt = IA64_FMT_M3;
                    break;
                case 0x24:
                    op = IA64_OP_LD1_C_NC, fmt = IA64_FMT_M3;
                    break;
                case 0x25:
                    op = IA64_OP_LD2_C_NC, fmt = IA64_FMT_M3;
                    break;
                case 0x26:
                    op = IA64_OP_LD4_C_NC, fmt = IA64_FMT_M3;
                    break;
                case 0x27:
                    op = IA64_OP_LD8_C_NC, fmt = IA64_FMT_M3;
                    break;
                case 0x28:
                    op = IA64_OP_LD1_C_CLR_ACQ, fmt = IA64_FMT_M3;
                    break;
                case 0x29:
                    op = IA64_OP_LD2_C_CLR_ACQ, fmt = IA64_FMT_M3;
                    break;
                case 0x2A:
                    op = IA64_OP_LD4_C_CLR_ACQ, fmt = IA64_FMT_M3;
                    break;
                case 0x2B:
                    op = IA64_OP_LD8_C_CLR_ACQ, fmt = IA64_FMT_M3;
                    break;
                case 0x30:
                    op = IA64_OP_ST1_, fmt = IA64_FMT_M5;
                    break;
                case 0x31:
                    op = IA64_OP_ST2_, fmt = IA64_FMT_M5;
                    break;
                case 0x32:
                    op = IA64_OP_ST4_, fmt = IA64_FMT_M5;
                    break;
                case 0x33:
                    op = IA64_OP_ST8_, fmt = IA64_FMT_M5;
                    break;
                case 0x34:
                    op = IA64_OP_ST1_REL, fmt = IA64_FMT_M5;
                    break;
                case 0x35:
                    op = IA64_OP_ST2_REL, fmt = IA64_FMT_M5;
                    break;
                case 0x36:
                    op = IA64_OP_ST4_REL, fmt = IA64_FMT_M5;
                    break;
                case 0x37:
                    op = IA64_OP_ST8_REL, fmt = IA64_FMT_M5;
                    break;
                case 0x3B:
                    op = IA64_OP_ST8_SPILL, fmt = IA64_FMT_M5;
                    break;
            }
            break;
        case 0x6:
            if (FIELD(bits, 27, 1) == 0) { /* x */
                switch (FIELD(bits, 30, 7)) { /* x6 + m */
                    case 0x0:
                        op = IA64_OP_LDFE_, fmt = IA64_FMT_M6;
                        break;
                    case 0x1:
                        op = IA64_OP_LDF8_, fmt = IA64_FMT_M6;
                        break;
                    case 0x2:
                        op = IA64_OP_LDFS_, fmt = IA64_FMT_M6;
                        break;
                    case 0x3:
                        op = IA64_OP_LDFD_, fmt = IA64_FMT_M6;
                        break;
                    case 0x4:
                        op = IA64_OP_LDFE_S, fmt = IA64_FMT_M6;
                        break;
                    case 0x5:
                        op = IA64_OP_LDF8_S, fmt = IA64_FMT_M6;
                        break;
                    case 0x6:
                        op = IA64_OP_LDFS_S, fmt = IA64_FMT_M6;
                        break;
                    case 0x7:
                        op = IA64_OP_LDFD_S, fmt = IA64_FMT_M6;
                        break;
                    case 0x8:
                        op = IA64_OP_LDFE_A, fmt = IA64_FMT_M6;
                        break;
                    case 0x9:
                        op = IA64_OP_LDF8_A, fmt = IA64_FMT_M6;
                        break;
                    case 0xA:
                        op = IA64_OP_LDFS_A, fmt = IA64_FMT_M6;
                        break;
                    case 0xB:
                        op = IA64_OP_LDFD_A, fmt = IA64_FMT_M6;
                        break;
                    case 0xC:
                        op = IA64_OP_LDFE_SA, fmt = IA64_FMT_M6;
                        break;
                    case 0xD:
                        op = IA64_OP_LDF8_SA, fmt = IA64_FMT_M6;
                        break;
                    case 0xE:
                        op = IA64_OP_LDFS_SA, fmt = IA64_FMT_M6;
                        break;
                    case 0xF:
                        op = IA64_OP_LDFD_SA, fmt = IA64_FMT_M6;
                        break;
                    case 0x1B:
                        op = IA64_OP_LDF_FILL, fmt = IA64_FMT_M6;
                        break;
                    case 0x20:
                        op = IA64_OP_LDFE_C_CLR, fmt = IA64_FMT_M6;
                        break;
                    case 0x21:
                        op = IA64_OP_LDF8_C_CLR, fmt = IA64_FMT_M6;
                        break;
                    case 0x22:
                        op = IA64_OP_LDFS_C_CLR, fmt = IA64_FMT_M6;
                        break;
                    case 0x23:
                        op = IA64_OP_LDFD_C_CLR, fmt = IA64_FMT_M6;
                        break;
                    case 0x24:
                        op = IA64_OP_LDFE_C_NC, fmt = IA64_FMT_M6;
                        break;
                    case 0x25:
                        op = IA64_OP_LDF8_C_NC, fmt = IA64_FMT_M6;
                        break;
                    case 0x26:
                        op = IA64_OP_LDFS_C_NC, fmt = IA64_FMT_M6;
                        break;
                    case 0x27:
                        op = IA64_OP_LDFD_C_NC, fmt = IA64_FMT_M6;
                        break;
                    case 0x2C:
                        op = IA64_OP_LFETCH_, fmt = IA64_FMT_M13;
                        break;
                    case 0x2D:
                        op = IA64_OP_LFETCH_EXCL, fmt = IA64_FMT_M13;
                        break;
                    case 0x2E:
                        op = IA64_OP_LFETCH_FAULT, fmt = IA64_FMT_M13;
                        break;
                    case 0x2F:
                        op = IA64_OP_LFETCH_FAULT_EXCL,
                        fmt = IA64_FMT_M13;
                        break;
                    case 0x30:
                        op = IA64_OP_STFE, fmt = IA64_FMT_M9;
                        break;
                    case 0x31:
                        op = IA64_OP_STF8, fmt = IA64_FMT_M9;
                        break;
                    case 0x32:
                        op = IA64_OP_STFS, fmt = IA64_FMT_M9;
                        break;
                    case 0x33:
                        op = IA64_OP_STFD, fmt = IA64_FMT_M9;
                        break;
                    case 0x3B:
                        op = IA64_OP_STF_SPILL, fmt = IA64_FMT_M9;
                        break;
                    case 0x40:
                        op = IA64_OP_LDFE_, fmt = IA64_FMT_M7;
                        break;
                    case 0x41:
                        op = IA64_OP_LDF8_, fmt = IA64_FMT_M7;
                        break;
                    case 0x42:
                        op = IA64_OP_LDFS_, fmt = IA64_FMT_M7;
                        break;
                    case 0x43:
                        op = IA64_OP_LDFD_, fmt = IA64_FMT_M7;
                        break;
                    case 0x44:
                        op = IA64_OP_LDFE_S, fmt = IA64_FMT_M7;
                        break;
                    case 0x45:
                        op = IA64_OP_LDF8_S, fmt = IA64_FMT_M7;
                        break;
                    case 0x46:
                        op = IA64_OP_LDFS_S, fmt = IA64_FMT_M7;
                        break;
                    case 0x47:
                        op = IA64_OP_LDFD_S, fmt = IA64_FMT_M7;
                        break;
                    case 0x48:
                        op = IA64_OP_LDFE_A, fmt = IA64_FMT_M7;
                        break;
                    case 0x49:
                        op = IA64_OP_LDF8_A, fmt = IA64_FMT_M7;
                        break;
                    case 0x4A:
                        op = IA64_OP_LDFS_A, fmt = IA64_FMT_M7;
                        break;
                    case 0x4B:
                        op = IA64_OP_LDFD_A, fmt = IA64_FMT_M7;
                        break;
                    case 0x4C:
                        op = IA64_OP_LDFE_SA, fmt = IA64_FMT_M7;
                        break;
                    case 0x4D:
                        op = IA64_OP_LDF8_SA, fmt = IA64_FMT_M7;
                        break;
                    case 0x4E:
                        op = IA64_OP_LDFS_SA, fmt = IA64_FMT_M7;
                        break;
                    case 0x4F:
                        op = IA64_OP_LDFD_SA, fmt = IA64_FMT_M7;
                        break;
                    case 0x5B:
                        op = IA64_OP_LDF_FILL, fmt = IA64_FMT_M7;
                        break;
                    case 0x60:
                        op = IA64_OP_LDFE_C_CLR, fmt = IA64_FMT_M7;
                        break;
                    case 0x61:
                        op = IA64_OP_LDF8_C_CLR, fmt = IA64_FMT_M7;
                        break;
                    case 0x62:
                        op = IA64_OP_LDFS_C_CLR, fmt = IA64_FMT_M7;
                        break;
                    case 0x63:
                        op = IA64_OP_LDFD_C_CLR, fmt = IA64_FMT_M7;
                        break;
                    case 0x64:
                        op = IA64_OP_LDFE_C_NC, fmt = IA64_FMT_M7;
                        break;
                    case 0x65:
                        op = IA64_OP_LDF8_C_NC, fmt = IA64_FMT_M7;
                        break;
                    case 0x66:
                        op = IA64_OP_LDFS_C_NC, fmt = IA64_FMT_M7;
                        break;
                    case 0x67:
                        op = IA64_OP_LDFD_C_NC, fmt = IA64_FMT_M7;
                        break;
                    case 0x6C:
                        op = IA64_OP_LFETCH_, fmt = IA64_FMT_M14;
                        break;
                    case 0x6D:
                        op = IA64_OP_LFETCH_EXCL, fmt = IA64_FMT_M14;
                        break;
                    case 0x6E:
                        op = IA64_OP_LFETCH_FAULT, fmt = IA64_FMT_M14;
                        break;
                    case 0x6F:
                        op = IA64_OP_LFETCH_FAULT_EXCL,
                        fmt = IA64_FMT_M14;
                        break;
                }
            } else {
                switch (FIELD(bits, 30, 7)) { /* x6 + m */
                    case 0x1:
                        op = IA64_OP_LDFP8_, fmt = IA64_FMT_M11;
                        break;
                    case 0x2:
                        op = IA64_OP_LDFPS_, fmt = IA64_FMT_M11;
                        break;
                    case 0x3:
                        op = IA64_OP_LDFPD_, fmt = IA64_FMT_M11;
                        break;
                    case 0x5:
                        op = IA64_OP_LDFP8_S, fmt = IA64_FMT_M11;
                        break;
                    case 0x6:
                        op = IA64_OP_LDFPS_S, fmt = IA64_FMT_M11;
                        break;
                    case 0x7:
                        op = IA64_OP_LDFPD_S, fmt = IA64_FMT_M11;
                        break;
                    case 0x9:
                        op = IA64_OP_LDFP8_A, fmt = IA64_FMT_M11;
                        break;
                    case 0xA:
                        op = IA64_OP_LDFPS_A, fmt = IA64_FMT_M11;
                        break;
                    case 0xB:
                        op = IA64_OP_LDFPD_A, fmt = IA64_FMT_M11;
                        break;
                    case 0xD:
                        op = IA64_OP_LDFP8_SA, fmt = IA64_FMT_M11;
                        break;
                    case 0xE:
                        op = IA64_OP_LDFPS_SA, fmt = IA64_FMT_M11;
                        break;
                    case 0xF:
                        op = IA64_OP_LDFPD_SA, fmt = IA64_FMT_M11;
                        break;
                    case 0x1C:
                        op = IA64_OP_SETF_SIG, fmt = IA64_FMT_M18;
                        break;
                    case 0x1D:
                        op = IA64_OP_SETF_EXP, fmt = IA64_FMT_M18;
                        break;
                    case 0x1E:
                        op = IA64_OP_SETF_S, fmt = IA64_FMT_M18;
                        break;
                    case 0x1F:
                        op = IA64_OP_SETF_D, fmt = IA64_FMT_M18;
                        break;
                    case 0x21:
                        op = IA64_OP_LDFP8_C_CLR, fmt = IA64_FMT_M11;
                        break;
                    case 0x22:
                        op = IA64_OP_LDFPS_C_CLR, fmt = IA64_FMT_M11;
                        break;
                    case 0x23:
                        op = IA64_OP_LDFPD_C_CLR, fmt = IA64_FMT_M11;
                        break;
                    case 0x25:
                        op = IA64_OP_LDFP8_C_NC, fmt = IA64_FMT_M11;
                        break;
                    case 0x26:
                        op = IA64_OP_LDFPS_C_NC, fmt = IA64_FMT_M11;
                        break;
                    case 0x27:
                        op = IA64_OP_LDFPD_C_NC, fmt = IA64_FMT_M11;
                        break;
                    case 0x41:
                        op = IA64_OP_LDFP8_, fmt = IA64_FMT_M12;
                        break;
                    case 0x42:
                        op = IA64_OP_LDFPS_, fmt = IA64_FMT_M12;
                        break;
                    case 0x43:
                        op = IA64_OP_LDFPD_, fmt = IA64_FMT_M12;
                        break;
                    case 0x45:
                        op = IA64_OP_LDFP8_S, fmt = IA64_FMT_M12;
                        break;
                    case 0x46:
                        op = IA64_OP_LDFPS_S, fmt = IA64_FMT_M12;
                        break;
                    case 0x47:
                        op = IA64_OP_LDFPD_S, fmt = IA64_FMT_M12;
                        break;
                    case 0x49:
                        op = IA64_OP_LDFP8_A, fmt = IA64_FMT_M12;
                        break;
                    case 0x4A:
                        op = IA64_OP_LDFPS_A, fmt = IA64_FMT_M12;
                        break;
                    case 0x4B:
                        op = IA64_OP_LDFPD_A, fmt = IA64_FMT_M12;
                        break;
                    case 0x4D:
                        op = IA64_OP_LDFP8_SA, fmt = IA64_FMT_M12;
                        break;
                    case 0x4E:
                        op = IA64_OP_LDFPS_SA, fmt = IA64_FMT_M12;
                        break;
                    case 0x4F:
                        op = IA64_OP_LDFPD_SA, fmt = IA64_FMT_M12;
                        break;
                    case 0x61:
                        op = IA64_OP_LDFP8_C_CLR, fmt = IA64_FMT_M12;
                        break;
                    case 0x62:
                        op = IA64_OP_LDFPS_C_CLR, fmt = IA64_FMT_M12;
                        break;
                    case 0x63:
                        op = IA64_OP_LDFPD_C_CLR, fmt = IA64_FMT_M12;
                        break;
                    case 0x65:
                        op = IA64_OP_LDFP8_C_NC, fmt = IA64_FMT_M12;
                        break;
                    case 0x66:
                        op = IA64_OP_LDFPS_C_NC, fmt = IA64_FMT_M12;
                        break;
                    case 0x67:
                        op = IA64_OP_LDFPD_C_NC, fmt = IA64_FMT_M12;
                        break;
                }
            }
            break;
        case 0x7:
            switch (FIELD(bits, 30, 6)) { /* x6 */
                case 0x0:
                    op = IA64_OP_LDFE_, fmt = IA64_FMT_M8;
                    break;
                case 0x1:
                    op = IA64_OP_LDF8_, fmt = IA64_FMT_M8;
                    break;
                case 0x2:
                    op = IA64_OP_LDFS_, fmt = IA64_FMT_M8;
                    break;
                case 0x3:
                    op = IA64_OP_LDFD_, fmt = IA64_FMT_M8;
                    break;
                case 0x4:
                    op = IA64_OP_LDFE_S, fmt = IA64_FMT_M8;
                    break;
                case 0x5:
                    op = IA64_OP_LDF8_S, fmt = IA64_FMT_M8;
                    break;
                case 0x6:
                    op = IA64_OP_LDFS_S, fmt = IA64_FMT_M8;
                    break;
                case 0x7:
                    op = IA64_OP_LDFD_S, fmt = IA64_FMT_M8;
                    break;
                case 0x8:
                    op = IA64_OP_LDFE_A, fmt = IA64_FMT_M8;
                    break;
                case 0x9:
                    op = IA64_OP_LDF8_A, fmt = IA64_FMT_M8;
                    break;
                case 0xA:
                    op = IA64_OP_LDFS_A, fmt = IA64_FMT_M8;
                    break;
                case 0xB:
                    op = IA64_OP_LDFD_A, fmt = IA64_FMT_M8;
                    break;
                case 0xC:
                    op = IA64_OP_LDFE_SA, fmt = IA64_FMT_M8;
                    break;
                case 0xD:
                    op = IA64_OP_LDF8_SA, fmt = IA64_FMT_M8;
                    break;
                case 0xE:
                    op = IA64_OP_LDFS_SA, fmt = IA64_FMT_M8;
                    break;
                case 0xF:
                    op = IA64_OP_LDFD_SA, fmt = IA64_FMT_M8;
                    break;
                case 0x1B:
                    op = IA64_OP_LDF_FILL, fmt = IA64_FMT_M8;
                    break;
                case 0x20:
                    op = IA64_OP_LDFE_C_CLR, fmt = IA64_FMT_M8;
                    break;
                case 0x21:
                    op = IA64_OP_LDF8_C_CLR, fmt = IA64_FMT_M8;
                    break;
                case 0x22:
                    op = IA64_OP_LDFS_C_CLR, fmt = IA64_FMT_M8;
                    break;
                case 0x23:
                    op = IA64_OP_LDFD_C_CLR, fmt = IA64_FMT_M8;
                    break;
                case 0x24:
                    op = IA64_OP_LDFE_C_NC, fmt = IA64_FMT_M8;
                    break;
                case 0x25:
                    op = IA64_OP_LDF8_C_NC, fmt = IA64_FMT_M8;
                    break;
                case 0x26:
                    op = IA64_OP_LDFS_C_NC, fmt = IA64_FMT_M8;
                    break;
                case 0x27:
                    op = IA64_OP_LDFD_C_NC, fmt = IA64_FMT_M8;
                    break;
                case 0x2C:
                    op = IA64_OP_LFETCH_, fmt = IA64_FMT_M15;
                    break;
                case 0x2D:
                    op = IA64_OP_LFETCH_EXCL, fmt = IA64_FMT_M15;
                    break;
                case 0x2E:
                    op = IA64_OP_LFETCH_FAULT, fmt = IA64_FMT_M15;
                    break;
                case 0x2F:
                    op = IA64_OP_LFETCH_FAULT_EXCL, fmt = IA64_FMT_M15;
                    break;
                case 0x30:
                    op = IA64_OP_STFE, fmt = IA64_FMT_M10;
                    break;
                case 0x31:
                    op = IA64_OP_STF8, fmt = IA64_FMT_M10;
                    break;
                case 0x32:
                    op = IA64_OP_STFS, fmt = IA64_FMT_M10;
                    break;
                case 0x33:
                    op = IA64_OP_STFD, fmt = IA64_FMT_M10;
                    break;
                case 0x3B:
                    op = IA64_OP_STF_SPILL, fmt = IA64_FMT_M10;
                    break;
            }
            break;
	}
    
	if (op != IA64_OP_NONE)
		return (ia64_extract(op, fmt, bits, b, slot));
	return (0);
}

/*
 * Decode X-unit instructions.
 */
static int
ia64_decodeX(const void *ip, struct ia64_bundle *b, int slot)
{
	uint64_t bits;
	enum ia64_fmt fmt;
	enum ia64_op op;
    
	bits = SLOT(ip, slot);
	fmt = IA64_FMT_NONE, op = IA64_OP_NONE;
	/* Initialize slot 1 (slot - 1) */
	b->b_inst[slot - 1].i_format = IA64_FMT_NONE;
	b->b_inst[slot - 1].i_bits = SLOT(ip, slot - 1);
    
	switch((int)OPCODE(bits)) {
        case 0x0:
            if (FIELD(bits, 33, 3) == 0) { /* x3 */
                switch (FIELD(bits, 27, 6)) { /* x6 */
                    case 0x0:
                        op = IA64_OP_BREAK_X, fmt = IA64_FMT_X1;
                        break;
                    case 0x1:
                        if (FIELD(bits, 26, 1) == 0) /* y */
                            op = IA64_OP_NOP_X, fmt = IA64_FMT_X5;
                        else
                            op = IA64_OP_HINT_X, fmt = IA64_FMT_X5;
                        break;
                }
            }
            break;
        case 0x6:
            if (FIELD(bits, 20, 1) == 0)
                op = IA64_OP_MOVL, fmt = IA64_FMT_X2;
            break;
        case 0xC:
            if (FIELD(bits, 6, 3) == 0) /* btype */
                op = IA64_OP_BRL_COND, fmt = IA64_FMT_X3;
            break;
        case 0xD:
            op = IA64_OP_BRL_CALL, fmt = IA64_FMT_X4;
            break;
	}
    
	if (op != IA64_OP_NONE)
		return (ia64_extract(op, fmt, bits, b, slot));
	return (0);
}

int
ia64_decode(const void *ip, struct ia64_bundle *b)
{
	const char *tp;
	unsigned int slot;
	int ok;
    
	memset(b, 0, sizeof(*b));
    
	b->b_templ = ia64_templname[TMPL(ip)];
	if (b->b_templ == 0)
		return (0);
    
	slot = 0;
	tp = b->b_templ;
    
	ok = 1;
	while (ok && *tp != 0) {
		switch (*tp++) {
            case 'B':
                ok = ia64_decodeB(ip, b, slot++);
                break;
            case 'F':
                ok = ia64_decodeF(ip, b, slot++);
                break;
            case 'I':
                ok = ia64_decodeI(ip, b, slot++);
                break;
            case 'L':
                ok = (slot++ == 1) ? 1 : 0;
                break;
            case 'M':
                ok = ia64_decodeM(ip, b, slot++);
                break;
            case 'X':
                ok = ia64_decodeX(ip, b, slot++);
                break;
            case ';':
                ok = 1;
                break;
            default:
                ok = 0;
                break;
		}
	}
	if (!ok)
		b->b_templ = 0;
	return (ok);
}

#include "exec.h"
#include "helpers.h"

/*****************************************************************************/
/* Softmmu support */
#if !defined (CONFIG_USER_ONLY)

#define MMUSUFFIX _mmu

#define SHIFT 0
#include "softmmu_template.h"

#define SHIFT 1
#include "softmmu_template.h"

#define SHIFT 2
#include "softmmu_template.h"

#define SHIFT 3
#include "softmmu_template.h"

/* try to fill the TLB and return an exception if error. If retaddr is
   NULL, it means that the function was called in C code (i.e. not
   from generated code or from helper.c) */
/* XXX: fix it to restore all registers */
void tlb_fill (target_ulong addr, int is_write, int mmu_idx, void *retaddr)
{
    TranslationBlock *tb;
    CPUState *saved_env;
    unsigned long pc;
    int ret;

    /* XXX: hack to restore env in all cases, even if not called from
       generated code */
    saved_env = env;
    env = cpu_single_env;
    ret = cpu_ia64_handle_mmu_fault(env, addr, is_write, mmu_idx, 1);
    if (unlikely(ret != 0)) {
        if (likely(retaddr)) {
            /* now we have a real cpu fault */
            pc = (unsigned long)retaddr;
            tb = tb_find_pc(pc);
            if (likely(tb)) {
                /* the PC is inside the translated code. It means that we have
                   a virtual CPU fault */
                cpu_restore_state(tb, env, pc, NULL);
            }
        }
        /* XXX */
        /* helper_raise_exception_err(env->exception_index, env->error_code); */
    }
    env = saved_env;
}

/* raise an exception */
void HELPER(exception)(uint32_t excp)
{
    env->exception_index = excp;
    cpu_loop_exit(env);
}

#endif
