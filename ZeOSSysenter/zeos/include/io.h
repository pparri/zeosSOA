/*
 * io.h - Definició de l'entrada/sortida per pantalla en mode sistema
 */

#ifndef __IO_H__
#define __IO_H__

#include <types.h>

#define CBUFFER_SIZE 256

/** Screen functions **/
/**********************/

Byte inb (unsigned short port);
void printc(char c);
void printc_xy(Byte x, Byte y, char c);
void printk(char *string);

struct c_Buffer
{
  unsigned char buffer[CBUFFER_SIZE];
  unsigned int rpointer;
  unsigned int wpointer;
  unsigned int Bwritten;
};

extern struct c_Buffer cBuffer;

#endif  /* __IO_H__ */
