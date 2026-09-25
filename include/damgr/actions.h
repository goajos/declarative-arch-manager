#ifndef DAMGR_ACTIONS_H
#define DAMGR_ACTIONS_H
#include "state.h"

typedef enum action_status { PENDING, SUCCEEDED, FAILED } Damgr_Action_Status;

typedef enum action_type {
  ROOT_SERVICE,
  PRE_ROOT_HOOK,
  PRE_USER_HOOK,
  PACKAGE,
  AUR_PACKAGE,
  USER_SERVICE,
  DOTFILE,
  POST_ROOT_HOOK,
  POST_USER_HOOK,
} Damgr_Action_Type;

typedef struct payload {
  char *name;
  Damgr_Darray packages;
} Damgr_Action_Payload;

typedef struct action {
  Damgr_Action_Payload payload;
  Damgr_Action_Status status;
  Damgr_Action_Type type;
  bool is_new_state;
} Damgr_Action;

typedef struct action_queue {
  Damgr_Action *items;
  size_t capacity;
  size_t count;
} Damgr_Action_Queue;

typedef struct actions {
  Damgr_Action_Queue *queues;
  size_t capacity;
  size_t count;
} Damgr_Actions;

int damgr_get_actions(Damgr_Actions *actions, Damgr_Config *old_config,
                      Damgr_Config *config);
int damgr_do_actions(char *user, Damgr_Config *old_config,
                     Damgr_Config *config);

#endif /* DAMGR_ACTIONS_H */
