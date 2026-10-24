#ifndef DAMGR_JOURNAL_H
#define DAMGR_JOURNAL_H

#include "damgr/paths.h"
#include "damgr/utils.h"

Result_t damgr_make_journal(Damgr_Paths *paths, struct stat *st,
                            const char *fidbuffer);

#ifdef DAMGR_JOURNAL_IMPLEMENTATION

Result_t damgr_make_journal(Damgr_Paths *paths, struct stat *st,
                            const char *fidbuffer) {
  bool exists;
  DAMGR_CALL(damgr_reg_exists_r(paths->journal, st, &exists));
  if (exists)
    return Error(
        damgr_err_fmt(".journal should not exist: %s", paths->journal));

  return Success;
}

#endif /* DAMGR_JOURNAL_IMPLEMENTATION */
#endif /* DAMGR_JOURNAL_H */
