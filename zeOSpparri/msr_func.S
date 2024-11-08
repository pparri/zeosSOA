#include <asm.h>
#include <segment.h>

#define TASK_STACK_OFFSET 1024

ENTRY(writeMSR)
	push %ebp
	movl %esp, %ebp
	movl 8(%ebp), %ecx		// offset --> ecx
	movl 12(%ebp), %eax		//edx --> eax: es la info que queremos guardar
	//forzamos cero
	movl $0, %edx		//msr[ecx] := edx:eax
	wrmsr
	movl %ebp, %esp
	pop %ebp
	ret

ENTRY(cambio_contexto)
	
	mov 4(%esp), %eax //cogemos la @kernel_esp del current
	mov %ebp, (%eax) //current()->kernel_esp = ebp
	mov 8(%esp), %esp // esp = new_esp
	pop %ebp
	//movl 0x10891, %esp
	ret
	

    /*	PILA
	
esp--> @ seguent inner  -- volvemos
        current_esp
		new_esp  
        ebx -- pop
        edi
        esi
        ebp
        */