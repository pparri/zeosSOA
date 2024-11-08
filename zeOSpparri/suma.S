#include <asm.h>

ENTRY(suma)
      push   %ebp             //adreça retorn 
      mov    %esp,%ebp        // ara podem operar amb ebp
      mov    0x8(%ebp),%edx
      mov    0xc(%ebp),%eax
      add    %edx,%eax
      pop    %ebp
      ret

