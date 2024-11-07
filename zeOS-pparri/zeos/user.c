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
  if (write(1,buffer,23) == -1) perror();

  //GETTIME
  buffer = "\nTesteando gettime:\n";
  if (write(1,buffer,20) == -1) perror();
  itoa(gettime(),buffer);
  if (write(1,buffer,1) == -1) perror();
  
  buffer = "\n";
  write(1,buffer,1);
  
  //GETPID
  buffer = "\nTesteando getpid:\n";
  if (write(1,buffer,19) == -1) perror();
  itoa(getpid(),buffer);
  write(1,buffer,1);
  
  //FORK & BLOCKS

  int rfork = fork();
  if (rfork == 0) 
  {
    block();
    buffer = "\nSoy el PROCESO: ";
    write(1,buffer,17);
    itoa(getpid(),buffer);
    write(1,buffer,2);
  }
  else 
  {
    buffer = "\nSoy el PROCESO: ";
    write(1,buffer,17);
    itoa(getpid(),buffer);
    write(1,buffer,2);
    //write(1,"hola",4);
  }
  
  while(1) 
  {
    //unblock(2);
    //itoa(getpid(),buffer);
    //write(1,buffer,2);
  }
}
