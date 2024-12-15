#include <libc.h>
void wrappersito_func(void(*func) (void*), void *param)
{
  func(param);
  //threadExit();
  //write (1,"esto no se tiene que imprimir", 30);
}