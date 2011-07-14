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
