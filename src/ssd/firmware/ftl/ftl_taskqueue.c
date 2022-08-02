#include "ftl_taskqueue.h"

#include "../config/mem.h"
#include "../lib/lock.h"
#include "../lib/dprint.h"
#include "ftl_nvme.h"
#include "ftl_core0.h"
// #include "ftl_core1.h"

ftl_task_queue_entry *ftl_task_queue;
u32 tosq_cnt = 0;
u32 not_tosq_cnt = 0;
void FTL_hcmd_print(host_cmd_entry *hcmd_entry)
{
	xil_printf("=========================================\n");
    xil_printf("op = %d\n", hcmd_entry->opcode);
    xil_printf("start lsn = %d\n", hcmd_entry->start_lsn);
    xil_printf("req num = %d\n", hcmd_entry->req_num);
    xil_printf("status = %d\n", hcmd_entry->status);
    xil_printf("-----------------------------------------\n");
}

/**********************************************************************************
Func    Name: FTL_int_task_queue
Descriptions: ��ʼ��task queue��һ����6��״̬��queue
Input   para: 
In&Out  Para: 
Output  para: 
Return value: �޷���ֵ
***********************************************************************************/
void FTL_int_task_queue(void)
{
    u32 i, j;
    ftl_task_queue = (ftl_task_queue_entry *)FTL_TASK_Q;
    for (i = 0; i < HCE_INIT; i++)
    {
        ftl_task_queue[i].head = 0;
        ftl_task_queue[i].tail = 0;
        ftl_task_queue[i].cnt = 0;
        for (j = 0; j < TASK_QUEUE_MAX; j++)
        {
            ftl_task_queue[i].queue[j] = NULL;
        }
        ftl_task_queue[i].status = i;
    }
//#ifdef FINISH
    ftl_task_queue[HCE_CHECK_CACHE].func = FTL_handle_checkcache;
    ftl_task_queue[HCE_DATAMOVED].func = FTL_handle_datamove;
    ftl_task_queue[HCE_FINISH].func = FTL_handle_finish;
    ftl_task_queue[HCE_FROM_CQ].func = FTL_handle_fromcq;
    ftl_task_queue[HCE_TO_SQ].func = FTL_handle_tosq;
    ftl_task_queue[HCE_WAIT_CQ].func = NULL;
//#endif
}

/**********************************************************************************
Func    Name: FTL_task_queue_is_empty
Descriptions: ���status״̬��taskqueue�Ƿ�Ϊ��
Input   para: 
In&Out  Para: 
Output  para: 
Return value: Ϊ�շ���TRUE,��Ϊ�շ���FAULS
***********************************************************************************/
static u8 FTL_task_queue_is_empty(host_cmd_entry_status status)
{
    return (ftl_task_queue[status].cnt == 0);
}

/**********************************************************************************
Func    Name: FTL_task_queue_is_full
Descriptions: ���status״̬��taskqueue�Ƿ�Ϊ��
Input   para: 
In&Out  Para: 
Output  para: 
Return value: Ϊ������TRUE,��Ϊ������FAULS
***********************************************************************************/
static u8 FTL_task_queue_is_full(host_cmd_entry_status status)
{
    return (ftl_task_queue[status].cnt == TASK_QUEUE_MAX);
}

/**********************************************************************************
Func    Name: FTL_task_queue_remove_head
Descriptions: ��status״̬��task queue��ȡ��ͷԪ�أ�������headֵ
Input   para: 
In&Out  Para: 
Output  para: 
Return value: ���в�Ϊ���򷵻���Ӧ��headԪ�أ�Ϊ���򷵻�NULL
to do: wait cq��move data��Ҫ����
***********************************************************************************/
host_cmd_entry *FTL_task_queue_remove_head(host_cmd_entry_status status)
{
    u8 old_head;
    u8 new_head;
    host_cmd_entry *hcmd = NULL;
    if (FTL_task_queue_is_empty(status) == FALSE)
    {
        ftl_task_queue[status].cnt--;
        old_head = ftl_task_queue[status].head;
        new_head = (old_head + 1) % TASK_QUEUE_MAX;
        ftl_task_queue[status].head = new_head;
        hcmd = ftl_task_queue[status].queue[old_head];
        ftl_task_queue[status].queue[old_head] = NULL;
        return hcmd;
    }
    else
    {
        DEBUG_PRINTF("%d status task queue is empty\n");
        return NULL;
    }
}

/**********************************************************************************
Func    Name: FTL_task_queue_add_tail
Descriptions: ��hcmd_entry���ӵ�status״̬queue��tail��
Input   para: 
In&Out  Para: 
Output  para: 
Return value: �ɹ�����SUCCESS,ʧ�ܷ���FAIL
to do: wait cq��move data��Ҫ����
***********************************************************************************/
u32 FTL_task_queue_add_tail(host_cmd_entry *hcmd_entry, host_cmd_entry_status status)
{
    u8 old_tail;
    u8 new_tail;

    if (FTL_task_queue_is_full(status) == FALSE)
    {
        ftl_task_queue[status].cnt++;
        old_tail = ftl_task_queue[status].tail;
        new_tail = (old_tail + 1) % TASK_QUEUE_MAX;
        ftl_task_queue[status].tail = new_tail;
        if (ftl_task_queue[status].queue[old_tail] != NULL)
        {
            assert(0);
        }
        ftl_task_queue[status].queue[old_tail] = hcmd_entry;
        return SUCCESS;
    }
    else
    {
        DEBUG_PRINTF("[FTL_task_queue_add_tail]%d status task queue is FULL\n", status);
        return FAIL;
    }
}

/**********************************************************************************
Func    Name: FTL_core0task_polling
Descriptions: polling core 0 �����е�taskqueue
Input   para: 
In&Out  Para: 
Output  para: 
Return value: ��
***********************************************************************************/
void FTL_core0task_polling(void)
{
//	xil_printf("ftl core0task polling 1\n");
    u32 i;
    u32 j;
    u32 cnt = 0;
    host_cmd_entry *hcmd_entry;

    static u32 cpl = 0;
    for (i = HCE_CHECK_CACHE; i <= HCE_FINISH; i++)
    {
//    	xil_printf("ftl core0task polling 2\n");
    	if(i == HCE_FROM_CQ)
    	{
//    		xil_printf("ftl core0task polling 3\n");
            get_spin_lock((u32 *)TQ_FROMCQ_SPIN_LOCK);
    		cnt = ftl_task_queue[i].cnt;
            release_spin_lock((u32 *)TQ_FROMCQ_SPIN_LOCK);
    	}
    	else
    	{
//    		xil_printf("ftl core0task polling 4\n");
    		cnt = ftl_task_queue[i].cnt;
    	}

        cnt = (cnt < MAX_POLLING_NUM) ? cnt : MAX_POLLING_NUM;
        if (i == HCE_TO_SQ)
        {
            tosq_cnt += cnt;
        }
        if (i == HCE_FINISH)
        {
            cpl += cnt;
            if ((cnt != 0) && (cpl % 1000 == 0))
            {
                EMU_log_println(LOG, "deal finish hcmd cnt %d", cpl);
            }
            if ((cnt != 0) && (cpl == 300000))
            {
                EMU_log_println(LOG, "deal finish hcmd cnt %d", cpl);
            }
        }
        for (j = 0; j < cnt; j++)
        {

            	if(i == HCE_FROM_CQ)
            	{
//            		xil_printf("ftl core0task polling 5.2\n");
                    get_spin_lock((u32 *)TQ_FROMCQ_SPIN_LOCK);
                    hcmd_entry = FTL_task_queue_remove_head((host_cmd_entry_status)i);
                    release_spin_lock((u32 *)TQ_FROMCQ_SPIN_LOCK);
            	}
            	else
            	{
//            		xil_printf("ftl core0task polling 5.3\n");
            		hcmd_entry = FTL_task_queue_remove_head((host_cmd_entry_status)i);
            	}



                if (hcmd_entry)
                {
//                	xil_printf("ftl core0task polling 5.4, status : %d, i = %d\n",hcmd_entry->status,i);
//                	FTL_hcmd_print(hcmd_entry);

                ftl_task_queue[i].func(hcmd_entry);
            }
            else
            {
                //                	xil_printf("ftl core0task polling 5.5\n");
                DEBUG_PRINTF("[FTL_core0task_polling] hcmd_entry NULL\n");
            }
        }
    }
    //    xil_printf("ftl core0task polling 6\n");
    //  if ((tosq_cnt % 4 == 0) && (tosq_cnt > 0))
    {
        FTL_polling_read_list();
        FTL_polling_write_list();
        //  tosq_cnt = 0;
    }
    /*
    else
    {
        not_tosq_cnt++;
        if (not_tosq_cnt == 100)
        {
            FTL_polling_read_list();
            FTL_polling_write_list();
            not_tosq_cnt = 0;
        }
    }
    */
}

/**********************************************************************************
Func    Name: FTL_core1task_polling
Descriptions: polling core 0 �����е�taskqueue
Input   para: 
In&Out  Para: 
Output  para: 
Return value: ��
to do: wait cq��move data��Ҫ����
***********************************************************************************/
void FTL_core1task_polling(void)
{
    #if 0
    u32 i;
    u32 j;
    u32 cnt;
    host_cmd_entry *hcmd_entry;
    for (i = HCE_WAIT_CQ + 1; i <= HCE_FINISH; i++)
    {
        cnt = ftl_task_queue[i].cnt;
        for (j = 0; j < cnt; j++)
        {
            hcmd_entry = FTL_task_queue_remove_head((host_cmd_entry_status)i);
            if (hcmd_entry)
            {
                if (i != HCE_WAIT_CQ)
                {
                    ftl_task_queue[i].func(hcmd_entry);
                }
            }
            else
            {
                DEBUG_PRINTF("[FTL_core1task_polling] hcmd_entry NULL\n");
            }
        }
    }
    #endif
}

u32 FTL_get_taskqueue_cnt(host_cmd_entry_status status)
{
    return ftl_task_queue[status].cnt;
}

/**********************************************************************************
Func    Name: FTL_sendhcmd
Descriptions: ��hcmd ����status״̬��taskqueue��
Input   para: 
In&Out  Para: 
Output  para: 
Return value: SUCCESS OR FAIL
 
***********************************************************************************/
u32 FTL_sendhcmd(host_cmd_entry *hcmd, host_cmd_entry_status status)
{
    u32 ret;
    host_cmd_entry_status old_status;
    old_status = hcmd->status;
    hcmd->status = status;
    if ((status > HCE_INIT) || (status < HCE_CHECK_CACHE))
    {
        DEBUG_PRINTF("[FTL_sendhcmd] status error\n");
        return FAIL;
    }
    else if ((HCE_CHECK_CACHE <= status) && (status < HCE_INIT))
    {
        ret = FTL_task_queue_add_tail(hcmd, status);
        return ret;
    }
}
