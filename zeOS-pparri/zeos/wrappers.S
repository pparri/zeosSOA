#include <asm.h>
#include <segment.h>

ENTRY(write)
	//preparar la pila
	
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
	pushl %ebp		//create a fake dynamic link
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
	movl %eax, errno	//errno es una pos mem
	movl $-1, %eax
	//poner ret en la cima
	addl $8, %esp
	movl %ebp, %esp
	popl %ebp
	ret
	
/////////////////////////////////////////////////////////////////////////////
ENTRY(gettime)

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

/////////////////////////////////////////////////////////////////////////////
ENTRY(getpid)

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
	movl %eax, errno	//errno es una pos mem
	movl $-1, %eax
	//poner ret en la cima
	addl $8, %esp
	movl %ebp, %esp
	popl %ebp
	ret
	
///////////////////////////////////////////////////////////////////////////////
ENTRY(fork)
	pushl %ebp
	movl %esp, %ebp
	
	
	movl $2, %eax
	pushl %ecx
	pushl %edx
	pushl $retadressF
	pushl %ebp		//create a fake dynamic link
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
	movl %eax, errno	//errno es una pos mem
	movl $-1, %eax
	addl $8, %esp
	movl %ebp, %esp
	popl %ebp
	ret
	
////////////////////////////////////////////////////////////////////////////
ENTRY(exit)
	pushl %ebp
	movl %esp, %ebp
	
	
	movl $1, %eax
	pushl %ecx
	pushl %edx
	pushl $retadressE
	pushl %ebp		//create a fake dynamic link
	movl %esp, %ebp
	
	SYSENTER
	
retadressE:
	popl %ebp
	addl $4, %esp
	popl %edx
	popl %ecx
	
	movl %ebp, %esp
	popl %ebp
	ret
	
//////////////////////////////////////////////////////////////////////////////
ENTRY(block)
	pushl %ebp
	movl %esp, %ebp
	
	
	movl $12, %eax
	pushl %ecx
	pushl %edx
	pushl $retadressB
	pushl %ebp		//create a fake dynamic link
	movl %esp, %ebp
	
	SYSENTER
	
retadressB:
	popl %ebp
	addl $4, %esp
	popl %edx
	popl %ecx
	
	movl %ebp, %esp
	popl %ebp
	ret
	
////////////////////////////////////////////////////////////////////////////////
ENTRY(unblock)
	pushl %ebp
	movl %esp, %ebp
	
	
	movl $13, %eax
	pushl %ecx
	pushl %edx
	movl 8(%ebp), %edx
	pushl $retadressUB
	pushl %ebp		//create a fake dynamic link
	movl %esp, %ebp
	
	SYSENTER
	
retadressUB:
	popl %ebp
	addl $4, %esp
	popl %edx
	popl %ecx
	
	cmpl $0, %eax
	jl trat_errorUB
	movl %ebp, %esp
	popl %ebp
	ret
	
trat_errorUB:
	not %eax	
	movl %eax, errno	//errno es una pos mem
	movl $-1, %eax
	addl $8, %esp
	movl %ebp, %esp
	popl %ebp
	ret


