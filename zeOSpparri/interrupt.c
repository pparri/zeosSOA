/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>
#include <stdint.h>

#include <zeos_interrupt.h>

Gate idt[IDT_ENTRIES];
Register    idtR;
unsigned int zeos_ticks = 0;
//debugging ts
#define TIMER_INTERVAL_MS 10
#define SWITCH_INTERVAL_MS 10000     
int timer_count = 0;                 
int toggle = 0; 

extern struct task_struct  *idle_task;
extern struct task_struct  *task_1;



char char_map[] =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','�','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','�',
  '\0','�','\0','�','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
  /* ***************************                                         */
  /* flags = x xx 0x110 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
  /* ********************                                                */
  /* flags = x xx 0x111 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);

  //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
  /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
     the system calls will be thread-safe. */
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}


void kbd_routine()
{
        char tecla = inb(0x60);   //inb coge el valor del registro 0x60 que recoge la información de la tecla pulsada en binario 
       // printk("Has pulsado la tecla: ");

        int key_code = tecla & 0x7f;  // 0111 1111
        int b = tecla >> 7;
        if (!b) {
          //if (char_map[key_code] == 'a') task_switch((union task_union *)idle_task);
          //if (char_map[key_code] == 'b') task_switch((union task_union *)task_1);

          if(char_map[key_code] == '\0') printc_xy(0,0,'C');
          else printc_xy(0,0,char_map[key_code]); //printc es para hacerlo a nivel de sistema
        }
}
void kbd_handler(void);


void pf_routine(unsigned int error, unsigned int eip)
{
  printk("\nError por page fault en: ");
  char hex_map[]="0123456789ABCDEF";
  printk("0x");
  for (int i = 28; i >= 0; i-=4) 
    printc(hex_map[(eip>>i)&0xf]);

  printk('\n');

  while(1);   //solo excepciones para no volver a modo usuario hasta que se arregle
}
void pf_handler(void);

void clock_routine() {
  zeos_show_clock();
  ++zeos_ticks;
  scheduler();
  /*
  //timer_count += TIMER_INTERVAL_MS;
  if (timer_count >= SWITCH_INTERVAL_MS) 
  {
  	printk("\nEntrada\n");
	//Alterna entre idle e init1
	if (toggle == 0) 
	{
	    task_switch(idle_task);
	    toggle = 1;
	}
	else 
	{
	    task_switch(task_1);
	    toggle = 0;
	}

	timer_count = 0;
   }
   
  */
   
}

void clock_handler(void);

void syscall_handler_sysenter(void);

void writeMSR(unsigned long msr, uint64_t value);


void setIdt()     //inicializa la IDT
{
  /* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;
  set_handlers();

  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */
  setInterruptHandler(33,kbd_handler,0);    // se encatga de inicializar a privilegio 0 el kbd_handler
  setInterruptHandler(14,pf_handler,0);    
  setInterruptHandler(32,clock_handler,0);    

  writeMSR(0x174, __KERNEL_CS); 
  writeMSR(0x175, INITIAL_ESP);  
  writeMSR(0x176, (unsigned)(int) syscall_handler_sysenter);
  set_idt_reg(&idtR);

}


