# 1 "msr_func.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "msr_func.S"
# 1 "include/asm.h" 1
# 2 "msr_func.S" 2
# 1 "include/segment.h" 1
# 3 "msr_func.S" 2



.globl writeMSR; .type writeMSR, @function; .align 0; writeMSR:
 push %ebp
 movl %esp, %ebp
 movl 8(%ebp), %ecx
 movl 12(%ebp), %eax

 movl $0, %edx
 wrmsr
 movl %ebp, %esp
 pop %ebp
 ret

.globl cambio_contexto; .type cambio_contexto, @function; .align 0; cambio_contexto:

 mov 4(%esp), %eax
 mov %ebp, (%eax)
 mov 8(%esp), %esp
 pop %ebp

 ret
