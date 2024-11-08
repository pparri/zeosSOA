#include <libc.h>
#include <errno.h>

char *buffer;
int pid;
int result = 0;

int suma(int par1, int par2);

int write(int fd,char *buffer, int size);  //wrapper

void perror(void);    // >= 0 quan correcte, sino < 0

 
 

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
 // result = suma(0x42, 0x666);
  //int *p = 0; *p = 0;

    //int fd = -1;
    //if (fd == -1) perror();
  
  //WRITE
  buffer = "\nWrite Funciona Bien!\n";
  if (write(1,buffer,23) == -1) {perror(); exit();}

  //GETTIME
  buffer = "\nTesteando gettime:\n";
  if (write(1,buffer,20) == -1) {perror(); exit();}
  itoa(gettime(),buffer);
  if (write(1,buffer,1) == -1) {perror(); exit();}
  
  buffer = "\n";
  write(1,buffer,1);
  
  //GETPID
  buffer = "\nTesteando getpid:\n";
  if (write(1,buffer,19) == -1) {perror(); exit();}
  itoa(getpid(),buffer);
  if (write(1,buffer,1) == -1) {perror(); exit();}
  
  //FORK & BLOCKS
  int rfork = fork();
  if (rfork == 0) 
  {
    int rfork2 = fork();
    if (rfork2 > 0)
    {
    	    //PROCESO 2 -> prueba de desbloqueo
	    buffer = "\nSoy el PROCESO: ";
	    if (write(1,buffer,17) == -1) {perror(); exit();}
	    itoa(getpid(),buffer);
	    if (write(1,buffer,2) ==-1) {perror(); exit();}
	    while(1)
	    {
	    	if (unblock(rfork2) == -1) {perror(); exit();}
	    }
     }
     else if (rfork2 == 0)
     {
            //PROCESO rfork2 -> prueba de bloqueo
            buffer = "\nSoy el PROCESO: ";
	    if (write(1,buffer,17) == -1) {perror(); exit();}
	    itoa(getpid(),buffer);
	    if (write(1,buffer,2) == -1) {perror(); exit();}
	    buffer = " y me voy a bloquear\n";
	    if (write(1,buffer,21) == -1) {perror(); exit();}
	    block();
	    buffer = "Me he desbloqueado y me voy!\n";
	    if (write(1,buffer,30) == -1) {perror(); exit();}
	    
	    //EXIT -> esto no deberia imprimir
	    exit();
	    buffer = "\nSoy el PROCESO: ";
	    if (write(1,buffer,17) == -1) {perror(); exit();}
	    
     }
     else perror();
  }
  else if (rfork > 0)
  {
    //PROCESO 1 -> Init
    buffer = "\nSoy el PROCESO: ";
    if (write(1,buffer,17) == -1) {perror(); exit();}
    itoa(getpid(),buffer);
    if (write(1,buffer,2) == -1) {perror(); exit();}
  }
  else {perror(); exit();}
  
  
  while(1) 
  {
    //itoa(getpid(),buffer);
    //write(1,buffer,2);
  }
}
