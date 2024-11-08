/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>


union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));

#if 1
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif

extern struct list_head blocked;
struct list_head freequeue;
struct list_head readyqueue;
struct task_struct * idle_task;
struct task_struct * task_1;
int qticks;

extern void main(void);




/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{	//init_idle va a saltar aqui --> no quiero que haga nada
	__asm__ __volatile__("sti": : :"memory");
	printk("idle process\n");
	while(1)
	{
	
	}
}


void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue)
{
	struct list_head * up = &(t->list);
	if (t->status == ST_RUN)
	{
		list_add_tail(up,dst_queue);
		t->status = ST_READY;
	}
	else if (t->status == ST_READY)
	{
		list_del(&t->list);
		t->status = ST_RUN;
	}
}

void sched_next_rr(void)
{
	struct task_struct * get_st;
	if(!list_empty(&readyqueue))
	{
		struct list_head * get_head = list_first(&readyqueue);
		get_st = list_head_to_task_struct(get_head);
		update_process_state_rr(get_st,NULL);
	}
	
	else get_st = idle_task;
	qticks = get_st->quantum;
	task_switch((union task_union *)get_st);
}

int needs_sched_rr(void)
{	
	if ((qticks == 0 && !list_empty(&readyqueue)) || (current()->status == ST_BLOCKED)) return 1;
	else return 0;
}

void update_sched_data_rr(void)
{
	qticks--;
}

void scheduler(void)
{
	update_sched_data_rr();
	if (needs_sched_rr())
	{
		struct task_struct * up = (struct task_struct*)current();
		update_process_state_rr(up, &readyqueue);
		sched_next_rr();
	}
}


void init_idle (void)
{	
	
	struct list_head *e = list_first(&freequeue);	//cogemos el primero de la freequeue
	//falta borrar-lo de la freequee
	list_del(e);
	struct task_struct *primer = list_head_to_task_struct(e);	//desde la posicion de la lista cogemos el task_struct del proceso
	primer->PID = 0;
	primer->quantum = 1000;
	primer->pending_unblocks = 0;
	primer->status = ST_READY;
	INIT_LIST_HEAD(&primer->kids);

	allocate_DIR(primer);	//inicializamos TP


	//contexto de ejecución
	union task_union * tuprimer = (union task_union*)primer; 	// el task union esta en la misma pos que la stack
	tuprimer->stack[KERNEL_STACK_SIZE-2] = (unsigned long) 0;		//0 de relleno para pop ebp
	tuprimer->stack[KERNEL_STACK_SIZE-1] = (unsigned long) cpu_idle;	//en donde queremos hacer el return
	tuprimer->task.kernel_esp = (unsigned long)&(tuprimer->stack[KERNEL_STACK_SIZE-2]); //kernel_esp = ebp
	idle_task = primer;	
	//comprovar valor de primer al inicialitzar el valor
	
	
}




void init_task1(void)
{
	struct list_head *l = list_first(&freequeue);	//cogemos el primero de la freequeue
	list_del(l);
	struct task_struct *primer = list_head_to_task_struct(l);
	union task_union * tuprimer = (union task_union*)primer;	//cogemos el task union
	primer->PID = 1;
	primer->quantum = 1000;
	primer->pending_unblocks = 0;
	primer->status = ST_RUN;
	INIT_LIST_HEAD(&primer->kids);

	allocate_DIR(primer);	//inicializamos TP

	set_user_pages(primer);	//mapea paginas fisicas y añade la traduccion logicas-fisicas
	tss.esp0 = KERNEL_ESP(tuprimer);	//esp0 tiene que apuntar al inicio de la pila
	writeMSR(0x175,(unsigned long) tss.esp0);	
	set_cr3(primer->dir_pages_baseAddr);	//actualizamos la TP del current al de primer

	task_1 = primer;
	
	
}

//codi temporal

int is_in_queue(struct list_head *queue, struct list_head *node) {
    struct list_head *pos;
    list_for_each(pos, queue) {
        if (pos == node) {
            return 1; 
        }
    }
    return 0; 
}



void init_sched()
{
	INIT_LIST_HEAD(&freequeue);		//inicializamos la freequeue vacia
	INIT_LIST_HEAD(&readyqueue);	//inicializamos la readyqueue
	INIT_LIST_HEAD(&blocked);
	
  	 for (int i = 0; i < NR_TASKS; ++i) 
  	 { 
		//union task_union t = task[i];	//añadimos todos los procesos posibles a la freequeue
		list_add(&(task[i].task.list),&freequeue);
  	 }
  	 qticks = 1000;
   
}

void inner_task_switch(union task_union *new)
{
	tss.esp0 = KERNEL_ESP(new);
	writeMSR(0x175,(int) tss.esp0);
	set_cr3(get_DIR(&new->task));	//actualizamos TP
	
	cambio_contexto(&current()->kernel_esp,new->task.kernel_esp);
}
//problema con el kernel_esp de new
struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

