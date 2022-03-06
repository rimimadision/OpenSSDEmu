#include "be.h"
#include "../../ftl/ftl_core1.h"
#include "../../lib/list.h"
#include "../../config/config.h"
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
polling_status stat[4];
list_head register_list = {&register_list, &register_list};
pthread_mutex_t fcl_be_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t list_cond = PTHREAD_COND_INITIALIZER;
void be_init()
{
    memset(stat, 0, sizeof(polling_status) * 4);
    stat[0].CQ_addr = HW_CH0_CQ_ENTRY_ADDR;
    stat[1].CQ_addr = HW_CH1_CQ_ENTRY_ADDR;
    stat[2].CQ_addr = HW_CH2_CQ_ENTRY_ADDR;
    stat[3].CQ_addr = HW_CH3_CQ_ENTRY_ADDR;
}

void *be()
{
    while (1)
    {
        pthread_mutex_lock(&fcl_be_mutex);
        while (!list_empty(&register_list))
        {
            pthread_cond_wait(&list_cond, &fcl_be_mutex);
        }
        container_of(register_list.next, )
        
        list_delete_head(&register_list, registered_sq, node);
        be_deal_register_sq(list_);

        pthread_mutex_unlock(&fcl_be_mutex);
    }
}

void be_set_sq(hw_queue_entry *sq_set, int ch, int ce)
{
    pthread_mutex_lock(&fcl_be_mutex);
    registered_sq *sq = malloc(sizeof(registered_sq));
    sq->ch = ch;
    memcpy(&sq->sq_entry, sq_set, sizeof(hw_queue_entry));
    list_add_tail(&register_list, &sq->node);
    pthread_cond_signal(&list_cond);
    pthread_mutex_unlock(&fcl_be_mutex);
}

void be_deal_register_sq(registered_sq *sq)
{
    
    free(sq);
}

void be_send_cq()
{
}