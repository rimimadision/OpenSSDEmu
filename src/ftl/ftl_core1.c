#include "ftl_core1.h"

#include "../lib/dprint.h"
#include "../lib/lock.h"
#include "../buffer/buffer.h"

#include "../ftl/ftl_taskqueue.h"
#include "../gc/gc.h"
#include "../emu/be/be.h"
#include <stdlib.h>

/**********************************************************************************
Func    Name: FTL_core1_main
Descriptions: core1ִ�е�ѭ��
Input   para: None
In&Out  Para: None
Output  para: None
Return value: None
***********************************************************************************/
void FTL_core1_main()
{
#ifndef EMU
	init_core1_mmu();

//	xil_printf("ftl_core1_init_mmu, ok!\n");
#endif
	while (1)
	{
		//		xil_printf("ftl_cq_polling, ok!\n");
		FTL_CQ_polling();
		//		xil_printf("ftl_cq_polling, end!\n");
		// FTL_core1task_polling();
	}
}

/**********************************************************************************
Func    Name: FTL_setup_core1
Descriptions: ����core1
Input   para: None
In&Out  Para: None
Output  para: None
Return value: None
***********************************************************************************/
void FTL_setup_core1()
{
#ifndef EMU
	__asm__ __volatile__(
		"ldr	r0,=0x41 \n"
		"mcr	p15,0,r0,c1,c0,1 \n"
		"dsb \n"
		"isb \n"
		:
		:
		: "cc");

	u32 core1_start_address = (u32)FTL_core1_main;
	Xil_Out32(0xfffffff0, core1_start_address);
	__asm__("sev");
	dmb();
#endif
}

/**********************************************************************************
Func    Name: FTL_CQ_polling
Descriptions: polling CQ
Input   para: None
In&Out  Para: None
Output  para: None
Return value: None
***********************************************************************************/
u32 FTL_CQ_polling()
{
#ifdef EMU
	static int c = 0;
	u32 ret = FAIL;
	pthread_mutex_lock(&fcl_be_mutex);
	// emu_log_println(LOG, "CQ here");
	list_node *next_node;
	u32 buf_id;
	u32 hcmd_id;
	u32 subpage;
	host_cmd_entry *hcmd_entry = NULL;
	for (list_node *head_node = complete_list.next; head_node != &complete_list; head_node = complete_list.next)
	{
		ret = SUCCESS;
		EMU_sq *sq = container_of(head_node, EMU_sq, node);
		buf_id = BUFF_get_buffer_id((sq->sq_entry.buf_adr) << 14);
		hcmd_id = INVALID_ID;
		if (sq == NULL)
		{
			assert(0);
		}
		if (buf_id >= BUFFER_NUMBER)
		{
			EMU_log_println(ERR, "   buff id %d,op %d,sq->sq_entry.buf_adr 0x%x \n ", buf_id, sq->sq_entry.op, sq->sq_entry.buf_adr);

			assert(0);
		}
		for (subpage = 0; subpage < PAGE_SIZE / SUB_PAGE_SIZE; subpage++)
		{

			hcmd_id = BUFF_get_hcmd_id(buf_id, subpage);
			if (hcmd_id != INVALID_ID)
			{
				hcmd_entry = HCL_get_hcmd_entry_addr(hcmd_id);

				if (sq->sq_entry.op == hcmd_entry->opcode)
				{
					hcmd_entry->cpl_cnt++;
					if (hcmd_entry->cpl_cnt == hcmd_entry->req_num)
					{
						//	EMU_log_println(DEBUG, "complete sq of hcmd %u", hcmd_entry->emu_id);

						FTL_sendhcmd(hcmd_entry, HCE_FROM_CQ);
					}
				}
				else if (hcmd_entry->opcode == 0)
				{
					EMU_log_println(ERR, "  hcmd %u,buff id %d,hcmd_entry->cpl_cnt %d\n ", hcmd_entry->emu_id, buf_id, hcmd_entry->cpl_cnt);

					assert(0);
				}
				else if ((sq->sq_entry.op == HCE_READ) && (hcmd_entry->opcode == HCE_WRITE))
				{
					//	EMU_log_println(LOG, "  pre read done ", hcmd_entry->emu_id, buf_id, hcmd_entry->cpl_cnt);

					//	EMU_log_println(ERR, "  hcmd %u,buff id %d,hcmd_entry->cpl_cnt %d \n", hcmd_entry->emu_id, buf_id, hcmd_entry->cpl_cnt);

					BUF_preread_done(buf_id);
					break;
				}
			}
		}
		list_delete_head(&complete_list);
		FCL_free_SQ_entry(sq->ch, sq->sq_index);
		free(sq);
	}

	pthread_mutex_unlock(&fcl_be_mutex);

	return SUCCESS;
#endif
}
