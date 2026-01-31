#include <stdio.h>
#include "../context.h"

void damngr_init()
{
    printf("hello from inside damngr_init...\n");
    
    init_context();
}
