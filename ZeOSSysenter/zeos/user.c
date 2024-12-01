#include <libc.h>

char buff[24];

int pid;

void put_hex(unsigned long num) 
{
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[16];
    int i = 0;

    if (num == 0) {
        char c = '0';
        write(1, &c, 1);
        return;
    }

    while (num > 0) {
        buffer[i++] = hex_chars[num % 16];
        num /= 16;
    }

    while (i-- > 0) {
        write(1, &buffer[i], 1);
    }
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  write(1, "\n", 1);
  char *region1 = sbrk(100);
  put_hex((unsigned long)region1);
  write(1, "\n", 1);
  char *region2 = sbrk(10); 
  put_hex((unsigned long)region2);
  write(1, "\n", 1);
  char *region4 = sbrk(1);
  put_hex((unsigned long)region4);
  write(1, "\n", 1);
  char *region3 = sbrk(-5); 
  put_hex((unsigned long)region3);
  write(1, "\n", 1);

  Sprite* sprite;
  int rows, cols;
  sbrk(rows*cols);
  sprite->x = rows;
  sprite->y = cols;
  char *spriteMatrix = {
      "############", "############", "############", "############", 
      "############", "############", "############", "############", 
      "############", "############", "############", "############", 
      "############", "############", "############", "############", 
      "############", "############", "############", "############", 
      "############", "############", "############", "############", 
      "############", "############", "############", "############", 
      "############", "############", "############", "############", 
      "############", "############", "############", "############", 
      "############", "############", "############", "############"
  };
  sprite->content = spriteMatrix;

  SetColor(1,5);

  char *b;
  while(1) { 
    /*
    if (getKey(b) == 1);
      write(b);
    */
    int rbytes = getKey(&b);
    if (rbytes > 0) write(1,&b,rbytes);
  }
}
