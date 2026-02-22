// #include "src/commands/init.c"
#include "src/commands/merge.c"
// #include "src/commands/update.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  if (argc == 1 || argc > 2) {
    puts("Not a valid damgr <command> parameter, possible commands:");
    // puts("\tdamgr init");
    puts("\tdamgr merge");
    // puts("\tdamgr update");
    // puts("\tdamgr validate");
    return EXIT_FAILURE;
  }

  int command_idx;
  if (memcmp(argv[1], "init", 4) == 0)
    command_idx = 0;
  else if (memcmp(argv[1], "merge", 5) == 0)
    command_idx = 1;
  else if (memcmp(argv[1], "update", 6) == 0)
    command_idx = 2;
  // else if (memcmp(argv[1], "validate", 8) == 0) command_idx = 3;
  else {
    puts("Not a valid damgr <command> parameter, possible commands:");
    // puts("\tdamgr init");
    puts("\tdamgr merge");
    // puts("\tdamgr update");
    // puts("\tdamgr validate");
    return EXIT_FAILURE;
  }

  int ret;
  switch (command_idx) {
  case 0:
    puts("starting damgr init...");
    // ret = damgr_init();
    break;
  case 1:
    puts("starting damgr merge...");
    if ((ret = damgr_merge()) != EXIT_SUCCESS) {
      fprintf(stderr, "exit status: %d\n", ret);
      return ret;
    }
    puts("damgr merge finished...");
    break;
  case 2:
    puts("starting damgr update...");
    // ret = damgr_update();
    break;
    // case 3:
    //     puts("damgr validate...");
    //     ret = 0;
    //     break;
  }

  puts("damgr finished!");
  return EXIT_SUCCESS;
}
