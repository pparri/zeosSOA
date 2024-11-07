# 0 "entry.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "entry.S"




# 1 "include/asm.h" 1
# 6 "entry.S" 2
# 1 "include/segment.h" 1
# 7 "entry.S" 2
# 77 "entry.S"
.globl kbd_handler; .type kbd_handler, @function; .align 0; kbd_handler:
        pushl %gs; pushl %fs; pushl %es; pushl %ds; pushl %eax; pushl %ebp; pushl %edi; pushl %esi; pushl %ebx; pushl %ecx; pushl %edx; movl $0x18, %edx; movl %edx, %ds; movl %edx, %es
        call kbd_routine
        movb $0x20, %al; outb %al, $0x20;
        pop %edx; pop %ecx; pop %ebx; pop %esi; pop %edi; pop %ebp; pop %eax; pop %ds; pop %es; pop %fs; pop %gs;
        iret

.globl pf_handler; .type pf_handler, @function; .align 0; pf_handler:




        call pf_routine



.globl clock_handler; .type clock_handler, @function; .align 0; clock_handler:
        pushl %gs; pushl %fs; pushl %es; pushl %ds; pushl %eax; pushl %ebp; pushl %edi; pushl %esi; pushl %ebx; pushl %ecx; pushl %edx; movl $0x18, %edx; movl %edx, %ds; movl %edx, %es
        movb $0x20, %al; outb %al, $0x20;
        call clock_routine
        pop %edx; pop %ecx; pop %ebx; pop %esi; pop %edi; pop %ebp; pop %eax; pop %ds; pop %es; pop %fs; pop %gs;
        iret

.globl syscall_handler_sysenter; .type syscall_handler_sysenter, @function; .align 0; syscall_handler_sysenter:
 push $0x2B
 push %EBP
 pushfl
 push $0x23
 push 4(%EBP)
 pushl %gs; pushl %fs; pushl %es; pushl %ds; pushl %eax; pushl %ebp; pushl %edi; pushl %esi; pushl %ebx; pushl %ecx; pushl %edx; movl $0x18, %edx; movl %edx, %ds; movl %edx, %es
 cmpl $0, %EAX
 jl sysenter_err
 cmpl $MAX_SYSCALL, %EAX
 jg sysenter_err
 call *sys_call_table(, %EAX, 0x04)
 jmp sysenter_fin
 sysenter_err:
 movl $-88, %EAX
 sysenter_fin:
 movl %EAX, 0x18(%ESP)
 pop %edx; pop %ecx; pop %ebx; pop %esi; pop %edi; pop %ebp; pop %eax; pop %ds; pop %es; pop %fs; pop %gs;
 movl (%ESP), %EDX
 movl 12(%ESP), %ECX
 sti
 sysexit

.globl task_switch; .type task_switch, @function; .align 0; task_switch:
        push %ebp
        mov %esp, %ebp

        push %esi
        push %edi
        push %ebx

        push 8(%ebp)
        call inner_task_switch
        addl $4, %esp
        pop %ebx
        pop %edi
        pop %esi

        pop %ebp

        ret
