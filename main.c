#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commands/update.c"

int main(int argc, char* argv[]) {
    if (argc == 1 || argc > 2) {
        puts("Not a valid damngr <command> parameter, possible commands:");
        puts("\tdamngr init");
        puts("\tdamngr validate");
        puts("\tdamngr sync");
        puts("\tdamngr update");
        return EXIT_FAILURE;
    } 
    
    int command_idx;
    if (memcmp(argv[1], "init", 4) == 0) command_idx = 0;
    else if (memcmp(argv[1], "validate", 8) == 0) command_idx = 1;
    else if (memcmp(argv[1], "sync", 4) == 0) command_idx = 2;
    else if (memcmp(argv[1], "update", 6) == 0) command_idx = 3;
    else {
        puts("Not a valid damngr <command> parameter, possible commands:");
        puts("\tdamngr init");
        puts("\tdamngr validate");
        puts("\tdamngr sync");
        puts("\tdamngr update");
        return EXIT_FAILURE;
    }

    int ret;
    switch (command_idx) {
        case 0:
            puts("damngr init...");
            ret = 0;
            break;
        case 1:
            puts("damngr validate...");
            ret = 0;
            break;
        case 2:
            puts("damngr sync...");
            ret = 0;
            break;
        case 3:
            puts("damngr update...");
            ret = damngr_update();
            break;
    }


    return ret;
}
