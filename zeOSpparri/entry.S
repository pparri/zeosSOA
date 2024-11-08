/*
 * entry.S - Entry point to system mode from user mode
*/

#include <asm.h>
#include <segment.h>

#define ENOSYS 88	/* Function not implemented */


/**************************************************/
/**** Save & Restore ******************************/
/**                                              **/
/** When we change to privilege level 0 (kernel) **/
/** (through an interrupt, a system call, an     **/
/** exception ...) we must save the state of the **/
/** currently running task (save).               **/
/**                                              **/
/** Stack layout in 'systemCall':                **/
/**                                              **/
/**   0(%esp) - %edx    \                        **/
/**   4(%esp) - %ecx     |                       **/
/**   8(%esp) - %ebx     |                       **/
/**   C(%esp) - %esi     | Register saved        **/
/**  10(%esp) - %edi     |  by 'save'            **/
/**  14(%esp) - %ebp     |                       **/
/**  18(%esp) - %eax     |                       **/
/**  1C(%esp) - %ds      |                       **/
/**  20(%esp) - %es      |                       **/
/**  24(%esp) - %fs      |                       **/
/**  28(%esp) - %gs     /                        **/
/**  2C(%esp) - %eip    \                        **/
/**  30(%esp) - %cs      |                       **/
/**  34(%esp) - %eflags  |  Return context saved **/
/**  38(%esp) - %oldesp  |   by the processor.   **/
/**  3C(%esp) - %oldss  /                        **/
/**                                              **/
/**************************************************/

#define SAVE_ALL \
      pushl %gs; \
      pushl %fs; \
      pushl %es; \
      pushl %ds; \
      pushl %eax; \
      pushl %ebp; \
      pushl %edi; \
      pushl %esi; \
      pushl %ebx; \
      pushl %ecx; \
      pushl %edx; \
      movl $__KERNEL_DS, %edx;    \
      movl %edx, %ds;           \
      movl %edx, %es    


#define RESTORE_ALL \
        pop %edx; \
        pop %ecx; \
        pop %ebx; \
        pop %esi; \
        pop %edi; \
        pop %ebp; \
        pop %eax; \
        pop %ds; \
        pop %es; \
        pop %fs; \
        pop %gs;


#define EOI \
movb $0x20, %al; \
outb %al, $0x20;



ENTRY(kbd_handler)
        SAVE_ALL               //guardamos registros                //end of interruption
        call kbd_routine 
        EOI     
        RESTORE_ALL                
        iret            

ENTRY(pf_handler)      

        //push 30(%esp)           //variable error
        //push 34(%esp)           
        //SAVE_ALL
        call pf_routine
        //RESTORE_ALL
        //iret

ENTRY(clock_handler)
        SAVE_ALL               //guardamos registros                   //end of interruption -- hemos modificado el orden
        EOI
        call clock_routine      
        RESTORE_ALL                
        iret      

ENTRY(syscall_handler_sysenter)
	push $__USER_DS
	push %EBP // User stack address
	pushfl          //flags
	push $__USER_CS
	push 4(%EBP) // User return address
	SAVE_ALL
	cmpl $0, %EAX           // identificador system call
	jl sysenter_err
	cmpl $MAX_SYSCALL, %EAX
	jg sysenter_err
	call *sys_call_table(, %EAX, 0x04)
	jmp sysenter_fin
	sysenter_err:
	movl $-ENOSYS, %EAX
	sysenter_fin:
	movl %EAX, 0x18(%ESP)           //resultado funcion
	RESTORE_ALL
	movl (%ESP), %EDX // Return address
	movl 12(%ESP), %ECX // User stack address
	sti // Enable interrupts again
	sysexit

ENTRY(task_switch)
        push %ebp
        mov %esp, %ebp

        push %esi
        push %edi
        push %ebx
        //mov 8(%ebp), %edx
        push 8(%ebp) //parametro task_union
        call inner_task_switch  
        addl $4, %esp // skipeamos parametro en la pila
        pop %ebx
        pop %edi
        pop %esi
        //movl %ebp, %esp
        pop %ebp
        
        ret

        /*
        ebx -- pop
        edi
        esi
       -- ebp
        ret
        c
        */

       /* INIT
        0x1cf88 <task+36744>:	0x0001d000
        0x1cf8c <task+36748>:	0x0000001e
        0x1cf90 <task+36752>:	0x00003ff4
        0x1cf94 <task+36756>:	0x000e0200
        0x1cf98 <task+36760>:	0x0001cfb8
       */

      /* TASK1
        0x1cf88 <task+36744>:	0x0001c000
        0x1cf8c <task+36748>:	0x00000030
        0x1cf90 <task+36752>:	0x00003ff4
        0x1cf94 <task+36756>:	0x000e0200
        0x1cf98 <task+36760>:	0x0001cfb8

      */


