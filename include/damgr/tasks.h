#ifndef DAMGR_TASKS_H
#define DAMGR_TASKS_H
#include "darray.h"
#include "state.h"
#include <stdlib.h>

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
  union {
    char *payload_name;
    Damgr_Darray packages;
  };
} Damgr_Task_Payload;

typedef struct task {
  Damgr_Task_Payload payload;
  Damgr_Task_Type type;
  bool is_positive;
} Damgr_Task;
Damgr_Task *task_constructor(Damgr_Task_Payload payload, Damgr_Task_Type type,
                             bool is_positive);

typedef enum status { PENDING, SUCCEEDED, FAILED } Damgr_Status;

typedef struct task_queue {
  Damgr_Darray tasks;
  Damgr_Status status;
} Damgr_Task_Queue;
Damgr_Task_Queue *task_queue_constructor(Damgr_Status status);

void damgr_get_task_queues_from_configs(Damgr_Config *old_config,
                                        Damgr_Config *config);
// void damgr_do_task_queues_for_config(Damgr_Config *config, bool is_positive);

#endif /* DAMGR_TASKS_H */
