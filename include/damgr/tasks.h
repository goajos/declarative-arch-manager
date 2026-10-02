#ifndef DAMGR_TASKS_H
#define DAMGR_TASKS_H
#include "darray.h"
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

typedef enum status { PENDING, SUCCEEDED, FAILED } Damgr_Status;

typedef struct module Damgr_Module; // forward declaration works for pointers?
typedef struct task_queue {
  Damgr_Task *tasks;
  size_t capacity;
  size_t count;
  char *queue_name;
  Damgr_Module *new_ptr; // points to new module
  Damgr_Module *old_ptr; // points to old module if present
  Damgr_Status status;
} Damgr_Task_Queue;

typedef struct host Damgr_Host; // forward declaration works for pointers?
typedef struct queues {
  Damgr_Task_Queue *queues;
  size_t capacity;
  size_t count;
  Damgr_Host *new_ptr; // points to new host
  Damgr_Host *old_ptr; // points to old host if present
} Damgr_Task_Queues;

typedef struct config Damgr_Config; // forward declaration works for pointers?
void damgr_get_tasks(Damgr_Config *old_config, Damgr_Config *config);

#endif /* DAMGR_TASKS_H */
