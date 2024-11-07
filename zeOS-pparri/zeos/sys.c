/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include "errno.h"



#define LECTURA 0
#define ESCRIPTURA 1

extern int zeos_ticks;
extern struct list_head freequeue, readyqueue, blocked;
extern struct task_struct * idle_task;
//unsigned int zeos_ticks = 0;
int sumPID = 2;

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int ret_from_fork()
{
	return 0;
}


int sys_fork()
{
  int PID=-1;

  // creates the child process
  if (list_empty(&freequeue)) return -ENOMEM;
  struct list_head * get_h = list_first(&freequeue);
  list_del(get_h);
  struct task_struct * new_kid = list_head_to_task_struct(get_h);
  	/*
  	void copy_data(void *start, void *dest, int size);
  	*/
  
  //copiamos task union del padre al hijo -> herencia datos sistema
  copy_data(current(), (union task_union*)new_kid, KERNEL_STACK_SIZE*4);
  
  //allocatamos nuevo page directory (inicializa TP)
  allocate_DIR(new_kid);
  
  /*
  	int alloc_frame( void )
	{
	    int i;
	    for (i=NUM_PAG_KERNEL; i<TOTAL_PAGES;) {
		if (phys_mem[i] == FREE_FRAME) {
		    phys_mem[i] = USED_FRAME;
		    return i;
		}
		i += 2; NOTE: There will be holes! This is intended. 
				DO NOT MODIFY
	    }

	    return -1;
	}
  */
  //cogemos las paginas fisicas necesarias para el data+stack
  //queremos que el hijo apunte a las mismas páginas físicas que el padre
  int new_ph_pag[NUM_PAG_DATA];
  for (int pag = 0; pag < NUM_PAG_DATA; ++pag)
  {
  	new_ph_pag[pag] = alloc_frame();
  	if (new_ph_pag < 0) 
  	{
  		//liberamos frame de anteriores procesos
  		for(int pagD = 0; pagD < pag; ++pagD) free_frame(new_ph_pag[pagD]);
  		//volvemos a encolar
  		list_add_tail(&((union task_union*)new_kid)->task.list, &freequeue);
  		return -EAGAIN;
  	}
  }
  
  //inicializamos child's space -> code
  page_table_entry * new_kidPT = get_PT(new_kid);
  page_table_entry * dadPT = get_PT(current());
 
  
  //Inherit user code
  for (int pag = 0; pag < NUM_PAG_CODE; ++pag)
  {
  	/* set_ss_pag - Associates logical page 'page' with physical page 'frame' -> TP rel */
  	/* get_frame - Returns the physical frame associated to page 'logical_page' */
  	//padre <- hijo (apuntan al mismo sitio fisico inicialmente (user c en este caso))
  	set_ss_pag(new_kidPT,pag+PAG_LOG_INIT_CODE,get_frame(dadPT,pag+PAG_LOG_INIT_CODE));
  }
  
  //Inherit system code
  for(int pag = 0; pag < NUM_PAG_KERNEL; ++pag) set_ss_pag(new_kidPT,pag+0,get_frame(dadPT,pag+0));
  
  
  //DATA+STACK
  //page table entries for the user data+stack (@L) have to point to the new allocated frames for this region.
  for (int pag = 0; pag < NUM_PAG_DATA; ++pag) set_ss_pag(new_kidPT,PAG_LOG_INIT_DATA+pag,new_ph_pag[pag]);
  
  int diff = NUM_PAG_CODE+NUM_PAG_DATA+NUM_PAG_KERNEL;
  //Grant temporary access to parent process (al data+stack del hijo) -> padre apunta temporalmente al data+stack del hijo (@f) para poder haer la copia
  for (int pag = 0; pag < NUM_PAG_DATA; ++pag)
  {
  	//padre @L debajo suya (debajo de todo donde data+stack + lo q ocupa esa data)
  	set_ss_pag(dadPT,pag+diff,new_ph_pag[pag]);
  	//copiamos la data -> copy_data((void *) KERNEL_START + *p_sys_size, (void*)L_USER_START, *p_usr_size);
  	//shift de 12 bits por los 4k
  	copy_data((void*)((NUM_PAG_KERNEL+pag) << 12),(void*)((pag+diff) << 12),PAGE_SIZE);
  	//borramos el puntero - /* del_ss_pag - Removes mapping from logical page 'logical_page' */
  	del_ss_pag(dadPT,pag+diff);
  }
  
  //flush de tlb ->disable access del padre al kid (puntero cr3 a tp del dad)
  set_cr3(get_DIR(current()));
  
  new_kid->PID = sumPID;
  new_kid->dad = current();
  PID = sumPID;
  ++sumPID;
  new_kid->status = ST_READY;
  new_kid->quantum = 1000;
  new_kid->pending_unblocks = 0;
  INIT_LIST_HEAD(&new_kid->kids);
  list_add_tail(&new_kid->list,&current()->kids);
  
  union task_union * c = (union task_union*)new_kid;
  
  //fake ebp
  c->stack[KERNEL_STACK_SIZE-19] = (unsigned long) 0;
  //return auxiliar para que eax sea 0
  c->stack[KERNEL_STACK_SIZE-18] = (unsigned long) ret_from_fork;
  //esp apunta al fake ebp (hara pop y encima de la pila quedara el ret from fork
  c->task.kernel_esp = (unsigned int)&(c->stack[KERNEL_STACK_SIZE-19]);
  
  list_add_tail(&c->task.list,&readyqueue);
  //task_switch(c);
  return PID;
}

void sys_exit()
{  
	struct task_struct * cs = (struct task_struct *)current();
	page_table_entry * curPT = get_PT(cs);
	//idle hereda hijos
	struct list_head * e = list_first(&cs->kids);
	if (e != NULL) 
	{
		list_for_each(e,&cs->kids)
		{	
			list_add_tail(e,&idle_task->kids);
			struct task_struct * eST = list_head_to_task_struct(e);
			//tiene un nuevo padre
			eST->dad = idle_task;
			list_del(e);
		}
	}
	//borrar frames
	for (int pag = 0; pag < NUM_PAG_DATA; ++pag)
	{
		int frame = get_frame(curPT,pag+PAG_LOG_INIT_DATA);
		free_frame(frame);
		del_ss_pag(curPT,pag+PAG_LOG_INIT_DATA);
	}
	
	list_add_tail(&current()->list,&freequeue);
	//++sumPID;
	sched_next_rr();
}

void sys_block(void)
{
	int pu = current()->pending_unblocks;
	if (pu == 0)
	{
		current()->status = ST_BLOCKED;
		list_add_tail(&current()->list,&blocked);
		scheduler();
	}
	else 
	{
		--current()->pending_unblocks;
	}
}

int sys_unblock(int pid)
{
	struct list_head * e;
	//comprobar que hijos bloqueados
	list_for_each(e,&current()->kids)
	{
		struct task_struct * eST = list_head_to_task_struct(e);
		if (eST->PID == pid)
		{
			if (eST->status == ST_BLOCKED) 
			{
				eST->status = ST_READY;
				//borramos el puntero a blocked
				list_del(&eST->list);
				list_add_tail(e,&readyqueue);
				return 0;
			}
			else 
			{
				current()->pending_unblocks++;
				return 0;
			}
		}
	}
	return -1;
}

char buffer_k[256];
#define BUFFER_SIZE 256

int sys_write(int fd, char * buffer, int size) {

	// Si el valor es 1, es error.
	int fd_error = check_fd(fd, ESCRIPTURA);
	if(fd_error) return fd_error; // Si es error, retornem error (valor negatiu amb codi error).

	if(buffer == NULL) return -EFAULT; // EFAULT
	if(size < 0) return -EINVAL; // EINVAL
	else if (buffer != NULL) {
		int es_ok = access_ok(VERIFY_WRITE,buffer, size);
		if (!es_ok) return -EFAULT;
	}

	int bytes = size;
	int written_bytes; 

	while(bytes > BUFFER_SIZE){
		copy_from_user(buffer+(size-bytes), buffer_k, BUFFER_SIZE);
		written_bytes = sys_write_console(buffer_k, BUFFER_SIZE);
		
		buffer = buffer+BUFFER_SIZE;
		bytes = bytes-written_bytes;
	}

	copy_from_user(buffer+(size-bytes), buffer_k, bytes);
	written_bytes = sys_write_console(buffer_k, bytes);
	bytes = bytes-written_bytes;	

	return size-bytes;
}

int sys_gettime()
{
	return zeos_ticks;
}
