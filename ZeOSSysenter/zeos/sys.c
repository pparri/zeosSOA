/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <p_stats.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

#define HEAP_SIZE 1024 // 1MB heap

int heap[HEAP_SIZE];
void *heap_end;

void * get_ebp();

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; 
  if (permissions!=ESCRIPTURA) return -EACCES; 
  return 0;
}

void user_to_system(void)
{
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
	return -ENOSYS; 
}

int sys_getpid()
{
	return current()->PID;
}

int sys_getKey(char *b)
{
  if (cBuffer.Bwritten == 0) return -1;
  else if (b == NULL) return -1;
  int rbytes;
  for (rbytes = 0; rbytes < CBUFFER_SIZE && rbytes < cBuffer.Bwritten; rbytes++)
  {
      b[rbytes] = cBuffer.buffer[cBuffer.rpointer];
      cBuffer.rpointer = (cBuffer.rpointer+1)%CBUFFER_SIZE;
  }
  cBuffer.Bwritten -= rbytes;
  return rbytes;
}
//inicializamos el heap después de la data
/*
----------
USER CODE
---------- data_start
USER DATA+STACK
---------- data_end
USER HEAP
----------
definido en mm.c
*/

void init_heap() {

int new_ph_pag, i;
  struct task_struct *c = current();
  page_table_entry *process_PT = get_PT(&c);
  int pag_log_heap = PAG_INIT_HEAP;
  new_ph_pag=alloc_frame();
  if (new_ph_pag!=-1) /* One page allocated */
  {//reservar a partir de la ultima pagina del heap
    set_ss_pag(process_PT, pag_log_heap , new_ph_pag);
  }
  else printk("No hay mas paginas disponibles\n");

  
}


//usamos void para aprovechar el ancho del bus
void *sys_sbrk(int size) {
  void *old_heap_end;
  void *new_heap_end;

  old_heap_end = heap_end; // actualizamos el anterior heap_end

  if (size > 0) {

    if ((heap_end + size) > (heap + HEAP_SIZE)) {
      // Si el actual heap_end es mayor que el heap_end_maximo, devuelve error
      return (void*) NULL;
    }

    //Si no es multiple de paginas, reservar una pagina extra
  int pag_size = size/PAGE_SIZE + (size%PAGE_SIZE? 1 : 0);

    //Reservar memoria en la zona de usuario para la nueva zona de memoria
    int new_ph_pag, i;
    page_table_entry *process_PT = get_PT(current());
    for (i=0; i<pag_size; i++)
    {
      new_ph_pag=alloc_frame();
      int pag_log_heap = (int)heap_end/PAGE_SIZE + ((int)heap_end%PAGE_SIZE? 1 : 0);
      if (new_ph_pag!=-1) /* One page allocated */
      {//reservar a partir de la ultima pagina del heap
        set_ss_pag(process_PT, pag_log_heap + i, new_ph_pag);
      }
      else return (void*) NULL;
    }
    //Actualizamos el end del heap
    new_heap_end = heap_end + size;
    heap_end = new_heap_end;
    return (void*)old_heap_end;
  }
  //Si es menor a cero, tenemos que desalojar la memoria reservada
  else if (size < 0) {
    //si heap_end - size es menor que @inicial de heap, overflow inicial
    if ((heap_end + size) < heap) {
      // Heap underflow
      return (void*) NULL;
    }
   //desalojar paginas fisicas
     int pag_size = -size/PAGE_SIZE + (size%PAGE_SIZE? 1 : 0);
      int pag_log_heap = (int)heap_end/PAGE_SIZE + ((int)heap_end%PAGE_SIZE? 1 : 0);
      //pagina logica del final del heap
    page_table_entry *process_PT = get_PT(current());
    int new_ph_pag, pag, i;
    for (i=0; i<pag_size; i++)
    {
      free_frame(get_frame(process_PT, pag_log_heap-i));
      del_ss_pag(process_PT, pag_log_heap-i);
    }

    new_heap_end = heap_end + size;
    heap_end = new_heap_end;
    return (void *)old_heap_end;
  }
  else { //si es cero devolvemos el heap_end actual
    // Return the current break
    return (void*)heap_end;
  } 


}

int global_PID=1000;

int ret_from_fork()
{
  return 0;
}

int sys_fork(void)
{
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;
  
  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;

  lhcurrent=list_first(&freequeue);
  
  list_del(lhcurrent);
  
  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);
  
  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  /* new pages dir */
  allocate_DIR((struct task_struct*)uchild);
  
  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    new_ph_pag=alloc_frame();
    if (new_ph_pag!=-1) /* One page allocated */
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
    }
    else /* No more free pages left. Deallocate everything */
    {
      /* Deallocate allocated pages. Up to pag. */
      for (i=0; i<pag; i++)
      {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
      }
      /* Deallocate task_struct */
      list_add_tail(lhcurrent, &freequeue);
      
      /* Return error */
      return -EAGAIN; 
    }
  }

  /* Copy parent's SYSTEM and CODE to child. */
  page_table_entry *parent_PT = get_PT(current());
  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag=0; pag<NUM_PAG_CODE; pag++)
  {
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
  }
  /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
  for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
  {
    /* Map one child page to parent's address space. */
    set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
    copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
  }
  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));

  uchild->task.PID=++global_PID;
  uchild->task.state=ST_READY;

  int register_ebp;		/* frame pointer */
  /* Map Parent's ebp to child's stack */
  register_ebp = (int) get_ebp();
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->task.register_esp=register_ebp + sizeof(DWord);

  DWord temp_ebp=*(DWord*)register_ebp;
  /* Prepare child stack for context switch */
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  uchild->task.state=ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);
  
  return uchild->task.PID;
}

#define TAM_BUFFER 512

int sys_write(int fd, char *buffer, int nbytes) {
char localbuffer [TAM_BUFFER];
int bytes_left;
int ret;

	if ((ret = check_fd(fd, ESCRIPTURA)))
		return ret;
	if (nbytes < 0)
		return -EINVAL;
	if (!access_ok(VERIFY_READ, buffer, nbytes))
		return -EFAULT;
	
	bytes_left = nbytes;
	while (bytes_left > TAM_BUFFER) {
		copy_from_user(buffer, localbuffer, TAM_BUFFER);
		ret = sys_write_console(localbuffer, TAM_BUFFER);
		bytes_left-=ret;
		buffer+=ret;
	}
	if (bytes_left > 0) {
		copy_from_user(buffer, localbuffer,bytes_left);
		ret = sys_write_console(localbuffer, bytes_left);
		bytes_left-=ret;
	}
	return (nbytes-bytes_left);
}


extern int zeos_ticks;

int sys_gettime()
{
  return zeos_ticks;
}

void sys_exit()
{  
  int i;

  page_table_entry *process_PT = get_PT(current());

  // Deallocate all the propietary physical pages
  for (i=0; i<NUM_PAG_DATA; i++)
  {
    free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
    del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
  }
  
  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);
  
  current()->PID=-1;
  
  /* Restarts execution of the next process */
  sched_next_rr();
}

/* System call to force a task switch */
int sys_yield()
{
  force_task_switch();
  return 0;
}

extern int remaining_quantum;

int sys_get_stats(int pid, struct stats *st)
{
  int i;
  
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT; 
  
  if (pid<0) return -EINVAL;
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=remaining_quantum;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}
