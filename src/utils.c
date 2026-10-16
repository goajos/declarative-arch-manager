#include "damgr/log.h"
#include <stdlib.h>
#include <sys/stat.h>

int damgr_check_dir(const char *dir) {
  struct stat st;
  if (stat(dir, &st) != 0)
    return EXIT_SUCCESS; // no directory
  // if (!S_ISDIR(st.st_mode)) {
  // }
  damgr_log(ERROR, "%s directory from a previous merge detected", dir);
  return EXIT_FAILURE;
}
