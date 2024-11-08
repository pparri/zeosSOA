/*
 * libc.c 
 */

#include <libc.h>
#include <errno.h>
#include <types.h>

int errno;

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}


int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

void perror(){
   char *bufferPerror;
	switch(errno)
	{
	    case EFAULT:
		      write(1,"@ mala\n",10);
		      break;
	    case EINVAL:
		      write(1,"Argumento invalido\n",19);
		      break;
	    case ENOSYS:
		      write(1,"Syscall no implementada\n",24);
		      break;
	    case EACCES:
		      write(1,"Falta de privilegios\n",21);
		      break;
	    case EBADF:
		      write(1,"Fichero erroneo\n",16);
		      break;
	    case ENOMEM:
		      write(1,"No hay procesos libres\n",23);
		      break;
            case EAGAIN:
		      write(1,"No quedan frames libres\n",24);
		      break;
	    default:
		      itoa(errno, bufferPerror);
		      write(1,"Error desconocido: ",19);
		      write(1,bufferPerror,strlen(bufferPerror));
          break;
  }
}

/*
void perror()
{
  char mesg[256];
  itoa(errno, mesg);

  write(1, mesg, strlen(mesg));
}

*/
