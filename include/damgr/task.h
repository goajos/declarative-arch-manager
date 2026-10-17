#ifndef DAMGR_TASK_H
#define DAMGR_TASK_H
#include "damgr/diff.h"

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
const char *task_type_string(Damgr_Task_Type type);

typedef enum task_status {
  PENDING,
  SUCCEEDED,
  FAILED,
  ROLLBACK
} Damgr_Task_Status;

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

#endif /* DAMGR_TASK_H */
