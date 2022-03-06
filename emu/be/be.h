#ifndef BE_H
#define BE_H

#include "../../lib/type.h"
#include "../../lib/list.h"

#define FLASH_LOG_NAME "flash_log"

extern list_head register_list;

typedef struct registered_sq
{
    int ch;
    hw_queue_entry sq_entry;
    list_node node;
}registered_sq;

void be_init();
void *be();
void be_register_sq();
void be_send_cq();

#endif