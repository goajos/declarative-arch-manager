#ifndef DAMGR_TASK_H
#define DAMGR_TASK_H
#include "damgr/cmd.h"
#include "damgr/diff.h"
#include "damgr/paths.h"

typedef enum task_type {
  PKG_INSTALL,
  PKG_REMOVE,
  AUR_INSTALL,
  AUR_REMOVE,
  SVC_ENABLE,
  SVC_DISABLE,
  LINK,
  UNLINK,
} Damgr_Task_Type;
const char *task_type_string(const Damgr_Task_Type type);

typedef enum task_status {
  PENDING,
  SUCCEEDED,
  FAILED,
  ROLLBACK_SUCCEEDED,
  ROLLBACK_FAILED
} Damgr_Task_Status;
const char *task_status_string(const Damgr_Task_Status status);

typedef struct task {
  Damgr_Task_Type type;
  const char *module_name;
  const char *name; // service/dotfile name, NULL for package tasks
  char **packages;  // package names, NULL for service/dotfile tasks
  size_t packages_cnt;
  bool privileged; // root vs user
  Damgr_Task_Status status;
} Damgr_Task;

// queue borrows memory from the arena, so the arena should always outlive it
typedef struct queue {
  Damgr_Task *tasks;
  size_t cnt;
  size_t capacity;
} Damgr_Task_Queue;

void damgr_free_queue(Damgr_Task_Queue *queue);

int damgr_fill_queue(Damgr_Task_Queue *queue, const Damgr_Host_Diff *host_diff,
                     Damgr_Arena *arena);

int do_task(const Damgr_Paths *paths, const char *aur_helper,
            const Damgr_Task *task);
int undo_task(const Damgr_Paths *paths, const char *aur_helper,
              const Damgr_Task *task);

#endif /* DAMGR_TASK_H */
