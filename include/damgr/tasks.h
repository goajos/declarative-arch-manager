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

typedef struct task_payload {
  char *payload_name;
  Damgr_Darray packages;
} Damgr_Task_Payload;

typedef enum task_status { PENDING, SUCCEEDED, FAILED } Damgr_Task_Status;

typedef struct task {
  Damgr_Task_Payload payload;
  Damgr_Task_Type type;
  Damgr_Task_Status status;
  bool is_positive;
} Damgr_Task;
Damgr_Task *task_constructor(Damgr_Task_Payload payload, Damgr_Task_Type type,
                             bool is_positive);

void damgr_get_module_task_queues_from_host(Damgr_Host *host);
void damgr_get_module_task_queues_from_hosts_diff(Damgr_Host *old_host,
                                                  Damgr_Host *host);
int damgr_do_module_task_queue(Damgr_Module *module, char *aur_helper);
void damgr_undo_module_task_queue(Damgr_Module *module);

#endif /* DAMGR_TASKS_H */
