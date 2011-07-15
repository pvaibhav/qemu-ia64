
/*
 * The pt_regs struct defines the way the registers are stored on
 * the stack during a system call.
 */

#define TARGET_NUM_GPRS        16

struct target_pt_regs {
    unsigned long pc;
    unsigned long code_ends_at;
};

#define UNAME_MACHINE "ia64"
