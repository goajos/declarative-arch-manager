#include "damgr/journal.h"

const char *journal_phase_string(Damgr_Journal_Phase phase) {
  switch (phase) {
  case DAMGR_JOURNAL_RUNNING:
    return "RUNNING";
  case DAMGR_JOURNAL_ROLLBACK:
    return "ROLLBACK";
  case DAMGR_JOURNAL_DONE:
    return "DONE";
  }
  return "";
}

static int write_journal_task(FILE *journal_fid, const size_t idx,
                              const Damgr_Task *task) {
  fprintf(journal_fid, "%zu\t%s\t%s\t%s\t%s", idx + 1,
          task_type_string(task->type),
          task->module_name != nullptr ? task->module_name : "-",
          task->name != nullptr ? task->name : "-",
          task_status_string(task->status));
  if (task->packages_cnt > 0) {
    for (size_t i = 0; i < task->packages_cnt; ++i)
      fprintf(journal_fid, "%s%s", i > 0 ? "," : "\t", task->packages[i]);
  } else {
    fprintf(journal_fid, "\t-");
  }
  fprintf(journal_fid, "\n");
  return ferror(journal_fid) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int damgr_write_journal(const char *journal_buf,
                        const Damgr_Journal_Phase phase,
                        const Damgr_Task_Queue *queue) {
  char tmp_buf[DAMGR_FID_BUF_MAX];
  snprintf(tmp_buf, sizeof(tmp_buf), "%s.tmp", journal_buf);
  FILE *tmp_fid = fopen(tmp_buf, "w");
  if (tmp_fid == nullptr) {
    damgr_log(ERROR, "could not open tmp file %s: %s", tmp_buf,
              strerror(errno));
    return EXIT_FAILURE;
  }

  fprintf(tmp_fid, "phase=%s\n", journal_phase_string(phase));

  int ret = EXIT_SUCCESS;
  for (size_t i = 0; i < queue->cnt && ret == EXIT_SUCCESS; ++i) {
    ret = write_journal_task(tmp_fid, i, &queue->tasks[i]);
    if (ret == EXIT_FAILURE) {
      damgr_log(ERROR, "could not write journal tmp file %s: %s", tmp_buf,
                strerror(errno));
      fclose(tmp_fid);
      unlink(tmp_buf);
      return ret;
    }
  }

  fclose(tmp_fid);
  if (rename(tmp_buf, journal_buf) != 0) {
    damgr_log(ERROR, "could not rename tmp file %s: %s", tmp_buf,
              strerror(errno));
    unlink(tmp_buf);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int damgr_read_journal(const char *journal_buf, int *phase, int *task_cnt) {
  FILE *fid = fopen(journal_buf, "r");
  if (fid == nullptr) {
    damgr_log(WARNING, "could not open journal %s", journal_buf);
    *phase = -1;
    *task_cnt = -1;
    return EXIT_FAILURE;
  }

  char line[DAMGR_LINE_MAX];
  int cur_phase = -1;
  int cnt = 0;
  while (fgets(line, sizeof(line), fid)) {
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\t')
      continue;
    if (strncmp(line, "phase=", 6) == 0) {
      char *val = line + 6;
      if (strncmp(val, "RUNNING", 7) == 0)
        cur_phase = DAMGR_JOURNAL_RUNNING;
      else if (strncmp(val, "ROLLBACK", 8) == 0)
        cur_phase = DAMGR_JOURNAL_ROLLBACK;
      else if (strncmp(val, "DONE", 4) == 0)
        cur_phase = DAMGR_JOURNAL_DONE;
    } else if (line[0] >= '0' && line[0] <= '9') {
      ++cnt;
    }
  }
  fclose(fid);

  if (cur_phase == -1) {
    damgr_log(ERROR, "journal %s is malformed, no phase header", journal_buf);
    *phase = -1;
    *task_cnt = -1;
    return EXIT_FAILURE;
  }
  *phase = cur_phase;
  *task_cnt = cnt;
  return EXIT_SUCCESS;
}

int damgr_clean_journal(const char *journal_buf) {
  if (unlink(journal_buf) != 0 && errno != ENOENT) {
    damgr_log(ERROR, "could not remove journal %s: %s", journal_buf,
              strerror(errno));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
