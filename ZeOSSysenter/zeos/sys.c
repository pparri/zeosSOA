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

extern struct sem_t semafors[10];

#define __USER_CS       0x23  /* 4 */
#define __USER_DS       0x2B  /* 5 */


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
  if (cBuffer.Bwritten == 0) return 0;
  else if (b == NULL) return -EINVAL;
  else if (!access_ok(VERIFY_WRITE, b, sizeof(char), NULL)) return -EFAULT;
  *b = cBuffer.buffer[cBuffer.rpointer];
  cBuffer.rpointer = (cBuffer.rpointer+1)%CBUFFER_SIZE;
  --cBuffer.Bwritten;
  return 1;
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

char * sys_sbrk(int size) 
{
    if (size == 0) return current()->heap_pointer_proc;

    char *old_pointer = current()->heap_pointer_proc;
    char *new_pointer = old_pointer + (size);
    union task_union* tu = (union task_union *) current();
    page_table_entry * PT = get_PT(current());

    if ((unsigned long)new_pointer < INIT_HEAP) return (char)NULL; //tiene que ser mayor a la direccion que usamos para la copia de datos y heap
    // ++
    if (size > 0) 
    {
      int p_log_nec = (size + PAGE_SIZE -1) / PAGE_SIZE; //redondear para arriba las paginas necesarias
      int paginas_reservadas = (current()->heap_pointer_proc - current()->heap_start_proc) / PAGE_SIZE;
      if (NUM_PAG_CODE + 2* NUM_PAG_DATA + NUM_PAG_KERNEL + p_log_nec + paginas_reservadas > TOTAL_PAGES) return (char) NULL; //quedan páginas logicas?
      while ((unsigned long)current()->heap_pointer_proc < (unsigned long)new_pointer) 
      {
        if ((unsigned long)current()->heap_pointer_proc % PAGE_SIZE == 0) { //si sobrepasamos límite de pagina reservamos una nueva
          int new_ph_pg = alloc_frame();
          if (new_ph_pg == -1) 
          {
              while ((unsigned long)current()->heap_pointer_proc > (unsigned long)old_pointer) 
              {
                  current()->heap_pointer_proc -= PAGE_SIZE;
                  free_frame(get_frame(PT, (unsigned long)current()->heap_pointer_proc/PAGE_SIZE));
                  del_ss_pag(PT, (unsigned long)current()->heap_pointer_proc/PAGE_SIZE);
              }
              return (char)NULL;
          }
          set_ss_pag(PT, (unsigned long)current()->heap_pointer_proc/PAGE_SIZE, new_ph_pg);
        }
        current()->heap_pointer_proc += PAGE_SIZE; 
        
      }
    }
    // --
    else 
    {
        while ((unsigned long)current()->heap_pointer_proc > (unsigned long)new_pointer) 
        {
            if ((unsigned long)current()->heap_pointer_proc % PAGE_SIZE == 0) {
               // Solo desalojar al inicio de una página
              if ((unsigned int)current()->heap_pointer_proc > DATA_END) {  // desalojar hasta el final de los datos
                del_ss_pag(PT, (unsigned long)current()->heap_pointer_proc/PAGE_SIZE);
                free_frame(get_frame(PT, (unsigned long)current()->heap_pointer_proc/PAGE_SIZE));
              }
            }
            current()->heap_pointer_proc -= PAGE_SIZE; 
        }

    }
    return old_pointer;
}

int sys_gotoXY(int posX, int posY)
{
  return cursor_move(posX,posY);
}

int sys_SetColor(int color, int background)
{
  return asthetic_change(color,background);
}

int sys_spritePut(int posX, int posY, Sprite* sp)
{
  return spriteDraw(posY,posX,sp);
}



void sys_threadExit(void) 
{
    
  page_table_entry *process_PT = get_PT(current());
    //Liberamos stack usuario
    free_frame(get_frame(process_PT, (unsigned int)current()->heap_pointer_proc/PAGE_SIZE)); 
    del_ss_pag(process_PT, (unsigned int)current()->heap_pointer_proc/PAGE_SIZE);
    current()->heap_pointer_proc -= PAGE_SIZE;
  
  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);
    
  /* Restarts execution of the next process */
  sched_next_rr();
    while (1) {}
}


int global_TID=1000;

int ret_from_thread() {
  return 0;
}

int sys_threadCreate(void (*function)(void*), void* parameter, void* wrapp) 
{

  if (!access_ok(VERIFY_READ, function, sizeof(void (*)(void*)), current())) {
    return -EFAULT;
}

if (parameter != NULL) {
    if (!access_ok(VERIFY_READ, parameter, sizeof(void*), current())) {
        return -EFAULT; 
    }
}

if (!access_ok(VERIFY_READ, wrapp, sizeof(void (*)(void*)), current())) {
    return -EFAULT;
}

    if (list_empty(&freequeue)) return -ENOMEM;


    struct list_head *lhcurrent = list_first(&freequeue);
    list_del(lhcurrent);

    //Alloc tcb

    union task_union *uchild = (union task_union *)list_head_to_task_struct(lhcurrent);
    
    //init tcb
    copy_data(current(), uchild, sizeof(union task_union));
    
    //TID + others
    uchild->task.TID = ++global_TID;
    uchild->task.PID = current()->PID;
    uchild->task.state = ST_READY;

    //Alloc user stack in current
    //reservar pagina fisica manualmente

    int pag_heap = ((unsigned long) (current()->heap_pointer_proc) / PAGE_SIZE); //304
    if (pag_heap > TOTAL_PAGES) return -1; //no hay mas paginas logicas
    page_table_entry *process_PT = get_PT(&uchild->task); //PT = 0x23000 ; page = 304; frame = 312
    int new_ph_pag=alloc_frame(); //312
    if (new_ph_pag!=-1) /* One page allocated */
    {
      set_ss_pag(process_PT, pag_heap, new_ph_pag);
      current()->heap_pointer_proc += PAGE_SIZE;
    }
    else return -1; //No hay paginas fisicas 
     //Configurar el stack de usuario

  uchild->task.ustack = (unsigned long *) (pag_heap<<12);   //direccion inicio user_stack (0x130000 -> 0x131000)

  uchild->task.ustack[1021] = (unsigned long)0x777; //   --> & =  0x130ff8 
  uchild->task.ustack[1022] = (unsigned long)function; //   
  uchild->task.ustack[1023] = (unsigned long)parameter; //   --> & =  0x130ffc


    // Configurar el contexto de kernel
    //unsigned long *kernel_stack = (unsigned long *)&uchild->stack[KERNEL_STACK_SIZE];
    uchild->stack[KERNEL_STACK_SIZE-2] = (unsigned long )&uchild->task.ustack[1021]; //esp apunta a fake ebp : 0x130ff8
    uchild->stack[KERNEL_STACK_SIZE-5] = (unsigned long)wrapp; //eip apunta a funcion : 0x100040
    
    uchild->task.register_esp = &uchild->stack[KERNEL_STACK_SIZE-18];  // = ebp : 0x19fb8
    

    init_stats(&(uchild->task.p_stats));

    //RQ
    list_add_tail(&(uchild->task.list), &readyqueue);

    //printk("hey");
    //task_switch(uchild);
    return uchild->task.TID;
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

  //Copiar datos del heap: Copia directa datos
  //regiones paginas para la Tp del padre
  int pagRegionIni = NUM_PAG_KERNEL+NUM_PAG_CODE;
  int pagRegionFin = NUM_PAG_KERNEL+NUM_PAG_CODE + NUM_PAG_DATA;
  char *pointer_current_start = current()->heap_start_proc;
  while (pointer_current_start < current()->heap_pointer_proc) {  //copiamos hasta donde llegue el heap
      for (i = pagRegionIni; i < pagRegionFin && pointer_current_start < current()->heap_pointer_proc; i++) {
        unsigned int ph_page = get_frame(parent_PT,(int)pointer_current_start/PAGE_SIZE); //cogemos el frame del padre
        set_ss_pag(process_PT,i+NUM_PAG_DATA, ph_page);  //asociamos con la TP del hijo
        copy_data((void*) (i << 12), (void*) ((i+NUM_PAG_DATA)<<12), PAGE_SIZE);
        del_ss_pag(parent_PT, i+NUM_PAG_DATA);
        pointer_current_start += PAGE_SIZE;
      }
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
	if (!access_ok(VERIFY_READ, buffer, nbytes, NULL))
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
  //desalojamos el heap 
  while(current()->heap_pointer_proc < current()->heap_start_proc) {
      free_frame(get_frame(process_PT, (unsigned int)current()->heap_pointer_proc/PAGE_SIZE)); 
      del_ss_pag(process_PT, (unsigned int)current()->heap_pointer_proc/PAGE_SIZE);
      current()->heap_pointer_proc -= PAGE_SIZE;
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
  
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats), NULL)) return -EFAULT; 
  
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

int sys_semCreate(int value) {
 for (int i = 0; i < 10; ++i) {
  if (semafors[i].semid == -1) {
    semafors[i].semid = i;  //semaforos van de 0 a 9
    semafors[i].count = value;
    semafors[i].TID = current()->PID; //tid del thread que lo ha creado
    INIT_LIST_HEAD(&semafors[i].blocked);
    //printk("creado");
    return i;
  }
 }
  return -1; //no hay mas semaforos disponibles en el proceso
}

int sys_semWait(int semID) {
  if (semID > 10 || semID < 0) return -1;
  if (semafors[semID].semid != semID) return -1;
  semafors[semID].count--;
  if (semafors[semID].count < 0) {
    printk("bloqueamos\n");
    current()->state = ST_BLOCKED;
    list_add(&current()->list,&semafors[semID].blocked);
    sched_next_rr();
  }
  return 0;
}

int sys_semSignal(int semID) {
  if (semID > 10 || semID < 0) return -1;
  if (semafors[semID].semid != semID) return -1;
  semafors[semID].count++;
  if (semafors[semID].count <= 0) { //hay almenos un thread bloqueado
    struct list_head *l = list_first(&semafors[semID].blocked);
    list_del(l);
    struct task_struct *unblocked_task = list_head_to_task_struct(l);
    unblocked_task->state = ST_READY;
    list_add(l,&readyqueue);
  }
  return 0;
}

int sys_semDestroy(int semID) {
  if (semID > 10 || semID < 0) return -1;
  if (semafors[semID].semid != semID) return -1;
  if (current()->TID != semafors[semID].TID) return -1; //solo el thread que lo ha creado puede destruirlo
  
  //desbloquear y nofiticar a los threads bloqueados del semaforo
  while (!list_empty(&semafors[semID].blocked)) {
      struct list_head *l = list_first(&semafors[semID].blocked);
      list_del(l);
      struct task_struct *blocked_thread = list_head_to_task_struct(l);
      blocked_thread->state = ST_READY;
      list_add_tail(&(blocked_thread->list), &readyqueue);
      printk("Desbloqueamos\n");
  }

    semafors[semID].count = NULL;
    semafors[semID].semid = -1;
    semafors[semID].TID = -1;
    return 0;
}
