#ifndef DAMGR_TASKS_H
#define DAMGR_TASKS_H
#include "state.h"

typedef enum task_status { PENDING, SUCCEEDED, FAILED } Damgr_Task_Status;

typedef enum task_type {
  ROOT_SERVICE,
  PRE_ROOT_HOOK,
  PRE_USER_HOOK,
  PACKAGE,
  AUR_PACKAGE,
  USER_SERVICE,
  DOTFILE,
  POST_ROOT_HOOK,
  POST_USER_HOOK,
} Damgr_Task_Type;

typedef struct payload {
  char *name;
  Damgr_Darray packages;
} Damgr_Task_Payload;

typedef struct task {
  Damgr_Task_Payload payload;
  Damgr_Task_Status status;
  Damgr_Task_Type type;
  // TODO: do we really need new state here?
  bool is_new_state;
} Damgr_Task;

typedef struct task_queue {
  Damgr_Task *items;
  size_t capacity;
  size_t count;
} Damgr_Task_Queue;

typedef struct tasks {
  Damgr_Task_Queue *queues;
  size_t capacity;
  size_t count;
} Damgr_Tasks;

int damgr_get_tasks(Damgr_Tasks *tasks, Damgr_Config *old_config,
                    Damgr_Config *config);
int damgr_do_tasks(char *user, Damgr_Config *old_config, Damgr_Config *config);

#endif /* DAMGR_TASKS_H */
