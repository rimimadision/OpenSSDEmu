#ifndef BE_H
#define BE_H

#include <pthread.h>

#include "../../lib/type.h"
#include "../../lib/list.h"
#include "../../fcl/fcl.h"
#define FLASH_LOG_NAME "flash_log"

extern list_head register_list;
extern list_head complete_list;
extern pthread_mutex_t fcl_be_mutex;
extern pthread_cond_t list_cond;
typedef struct registered_sq
{
    int ch;
    u32 sq_index;
    hw_queue_entry sq_entry;
    list_node node;
}registered_sq;

void be_init();
void *be();
void be_deal_register_sq(u32 sq_index, registered_sq *sq);
void be_set_sq(u32 sq_index, hw_queue_entry *sq_set, int ch, int ce);
#endif