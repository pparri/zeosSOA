#include <libc.h>
void wrappersito_func(void(*func) (void*), void *param)
{
  func(param);
  threadExit();
  while(1);
  //Nos llegan dos nulls por eso da pagefault :')
}