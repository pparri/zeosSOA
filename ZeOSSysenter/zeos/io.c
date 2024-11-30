/*
 * io.c - 
 */

#include <io.h>

#include <types.h>

#include <errno.h>

/**************/
/** Screen  ***/
/**************/

#define NUM_COLUMNS 80
#define NUM_ROWS    25

struct c_Buffer cBuffer = {{0}, 0, 0, 0};

Byte x, y=19;
int textColor = 2;
int textBackground = 0;

/* Read a byte from 'port' */
Byte inb (unsigned short port)
{
  Byte v;

  __asm__ __volatile__ ("inb %w1,%0":"=a" (v):"Nd" (port));
  return v;
}

void printc(char c)
{
     __asm__ __volatile__ ( "movb %0, %%al; outb $0xe9" ::"a"(c)); /* Magic BOCHS debug: writes 'c' to port 0xe9 */
  if (c=='\n')
  {
    x = 0;
    y=(y+1)%NUM_ROWS;
  }
  else
  {
    Word ch = (Word) (c & 0x00FF) | (textColor << 8) | (textBackground << 12);
	Word *screen = (Word *)0xb8000;
	screen[(y * NUM_COLUMNS + x)] = ch;
    if (++x >= NUM_COLUMNS)
    {
      x = 0;
      y=(y+1)%NUM_ROWS;
    }
  }
}

void printc_xy(Byte mx, Byte my, char c)
{
  Byte cx, cy;
  cx=x;
  cy=y;
  x=mx;
  y=my;
  printc(c);
  x=cx;
  y=cy;
}

void printk(char *string)
{
  int i;
  for (i = 0; string[i]; i++)
    printc(string[i]);
}

/* CURSOR */
int cursor_move(int posX, int posY)
{
  if ((posX > NUM_ROWS || posX < 0) || (posY > NUM_COLUMNS || posY < 0)) return -EINVAL;
  x = posX;
  y= posY;
  return 0;
}

/* COLOR */
int asthetic_change(int color, int background)
{
  //incluyendo bright colors
  if ((color > 15 || color < 0) || (background > 15 || background < 0)) return -EINVAL;
  textColor = color;
  textBackground = background;
  return 0;
}

/* SPRITE */
int spriteDraw(int posX, int posY, Sprite *sp)
{
  //falta chekear
  /*
  for (int i = posX; i < sp.x; ++i)
  {
    for (int j = posY; j < sp.y; ++j)
    {
      printc_xy(sp.content[i][j]);
      --pposy;
    }
  }
  */
}