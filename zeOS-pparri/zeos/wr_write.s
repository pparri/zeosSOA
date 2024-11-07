# 1 "wr_write.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "wr_write.S"
# 1 "include/asm.h" 1
# 2 "wr_write.S" 2
# 1 "include/segment.h" 1
# 3 "wr_write.S" 2

.globl write; .type write, @function; .align 0; write:


 pushl %ebp
 movl %esp, %ebp
 pushl %ebx
 pushl %ecx
 pushl %edx

 movl $4, %eax
 movl 8(%ebp), %edx;
 movl 12(%ebp), %ecx;
 movl 16(%ebp), %ebx;

 pushl $retadress
 pushl %ebp
 movl %esp, %ebp

 SYSENTER

retadress:
 popl %ebp
 addl $4, %esp
 popl %edx
 popl %ecx
 popl %ebx

 cmpl $0, %eax
 jl trat_error
 movl %ebp, %esp
 popl %ebp
 ret

trat_error:
 not %eax
 movl %eax, errno
 movl $-1, %eax

 addl $8, %esp
 movl %ebp, %esp
 popl %ebp
 ret


.globl gettime; .type gettime, @function; .align 0; gettime:

 pushl %ebp
 movl %esp, %ebp
 movl $10, %eax
 pushl %ecx
 pushl %edx
 pushl $retadressT
 pushl %ebp
 movl %esp, %ebp

 SYSENTER

retadressT:
 movl %ebp, %esp
 popl %ebp
 addl $4, %esp
 popl %edx
 popl %ecx
 movl %ebp, %esp
 popl %ebp
 ret


.globl getpid; .type getpid, @function; .align 0; getpid:

 pushl %ebp
 movl %esp, %ebp
 movl $20, %eax
 pushl %ecx
 pushl %edx

 pushl $retadressP
 pushl %ebp
 movl %esp, %ebp

 SYSENTER

retadressP:
 popl %ebp
 addl $4, %esp
 popl %edx
 popl %ecx

 cmpl $0, %eax
 jl trat_errorP
 movl %ebp, %esp
 popl %ebp
 ret

trat_errorP:
 not %eax
 movl %eax, errno
 movl $-1, %eax

 addl $8, %esp
 movl %ebp, %esp
 popl %ebp
 ret


.globl fork; .type fork, @function; .align 0; fork:
 pushl %ebp
 movl %esp, %ebp


 movl $2, %eax
 pushl %ecx
 pushl %edx
 pushl $retadressF
 pushl %ebp
 movl %esp, %ebp

 SYSENTER

retadressF:
 popl %ebp
 addl $4, %esp
 popl %edx
 popl %ecx

 cmpl $0, %eax
 jl trat_errorF
 movl %ebp, %esp
 popl %ebp
 ret

trat_errorF:
 not %eax
 movl %eax, errno
 movl $-1, %eax
 addl $8, %esp
 movl %ebp, %esp
 popl %ebp
 ret
