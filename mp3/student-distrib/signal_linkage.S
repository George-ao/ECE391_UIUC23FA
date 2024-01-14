#define ASM

.globl call_sig_return

call_sig_return:
    movl    $10, %eax
    int     $0x80
    RET

