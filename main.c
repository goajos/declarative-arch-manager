#define _DEFAULT_SOURCE
#include <stdio.h>
#include "context.h"
#include "commands/init.c"
#include "commands/validate.c"
#include "commands/sync.c"
#include "commands/update.c"


int main(int argc, char *argv[])
{
    if (argc == 1 || argc > 2) {
        printf("Print help...\n"); // TODO: wrong amount of cli arguments
        return 0;
    }
    
    int command_idx;
    if (string_equal(String(argv[1]), String("init"))) command_idx = 0; 
    else if (string_equal(String(argv[1]), String("validate"))) command_idx = 1;
    else if (string_equal(String(argv[1]), String("sync"))) command_idx = 2;
    else if (string_equal(String(argv[1]), String("update"))) command_idx = 3;
    else {
        printf("Print help...\n"); // TODO: unsupported command argument
        return 0;
    }

    Context context = {
        .hosts =        {0},
        .modules =      {0},
        .packages =     {0},
        .aur_helper =   { .data=nullptr },
        .aur_packages = {0},
        .services =     {0},
        .hooks =        {0}
    };

    switch(command_idx) {
        case 0:
            printf("damngr init called...\n");
            damngr_init();
            break;
        case 1:
            printf("damngr validate called...\n");
            damngr_validate();
            break;
        case 2:
            printf("damngr sync called...\n");
            damngr_sync();
            break;
        case 3:
            printf("damngr update called...\n");
            context = damngr_update(context);
            break;
    }

    free_context(context);

    return 0;
}
