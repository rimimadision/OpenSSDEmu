#include "be.h"
#include "../../ftl/ftl_core1.h"
#include "../../lib/list.h"
#include "../../config/config.h"
#include "../../emu/emu_log.h"
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
/* completely same as my_polling_status in ftl_core1 */
list_head register_list = {&register_list, &register_list};
list_head complete_list = {&complete_list, &complete_list};
pthread_mutex_t fcl_be_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t list_cond = PTHREAD_COND_INITIALIZER;
void be_init()
{
}

void *be()
{
    while (1)
    {
        pthread_mutex_lock(&fcl_be_mutex);
        while (list_empty(&register_list))
        {
            pthread_mutex_unlock(&fcl_be_mutex);
            continue;
            //pthread_cond_wait(&list_cond, &fcl_be_mutex);
        }
        registered_sq *r_sq = container_of(register_list.next, registered_sq, node);
        list_delete_head(&register_list);
        pthread_mutex_unlock(&fcl_be_mutex);
        be_deal_register_sq(r_sq->sq_index, r_sq);
    }
}

void be_set_sq(u32 sq_index, hw_queue_entry *sq_set, int ch, int ce)
{
    pthread_mutex_lock(&fcl_be_mutex);
    registered_sq *sq = malloc(sizeof(registered_sq));
    sq->ch = ch;
    sq->sq_index = sq_index;
    memcpy(&sq->sq_entry, sq_set, sizeof(hw_queue_entry));
    list_add_tail(&sq->node, &register_list);
    // pthread_cond_signal(&list_cond);
    pthread_mutex_unlock(&fcl_be_mutex);
}

void be_deal_register_sq(u32 sq_index, registered_sq *sq)
{
    pthread_mutex_lock(&fcl_be_mutex);
    emu_log_println(LOG, "sq of hcmd %d", sq->sq_entry.hcmd_index);
    list_add_tail(&sq->node, &complete_list);
    pthread_mutex_unlock(&fcl_be_mutex);
}
