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
	pthread_mutex_lock(&fcl_be_mutex);
	// emu_log_println(LOG, "CQ here");
	list_node *next_node;

	for (list_node *head_node = complete_list.next; head_node != &complete_list; head_node = complete_list.next)
	{
		EMU_sq *sq = container_of(head_node, EMU_sq, node);
		FCL_free_SQ_entry(sq->ch, sq->sq_index);
		host_cmd_entry *hcmd_entry = HCL_get_hcmd_entry_addr(sq->sq_entry.hcmd_index);
		EMU_log_println(DEBUG, "complete sq of hcmd %u",hcmd_entry->emu_id);
		hcmd_entry->cpl_cnt++;
		if (hcmd_entry->cpl_cnt == hcmd_entry->req_num)
		{
			FTL_sendhcmd(hcmd_entry, HCE_FROM_CQ);
		}
			
		list_delete_head(&complete_list);
		free(sq);
	}

	pthread_mutex_unlock(&fcl_be_mutex);
#else

	static u32 cqpolling = 0;
	//	if (cqpolling%10 == 0) {
	//		xil_printf("ftl_cq_polling start!\n");
	//	}
	cqpolling++;

	static polling_status my_polling_status[4] = {0};

	my_polling_status[0].CQ_addr = HW_CH0_CQ_ENTRY_ADDR;
	my_polling_status[1].CQ_addr = HW_CH1_CQ_ENTRY_ADDR;
	my_polling_status[2].CQ_addr = HW_CH2_CQ_ENTRY_ADDR;
	my_polling_status[3].CQ_addr = HW_CH3_CQ_ENTRY_ADDR;

	//#include"xtime_l.h"
	// XTime tmp_test_start_time = 0;
	// XTime tmp_test_end_time = 0;

	for (u32 i = 0; i < 4; i++)
	{
		u32 CQ_entry_addr = my_polling_status[i].CQ_addr + my_polling_status[i].polling_index * 16;
		hw_queue_entry *CQ_entry = (hw_queue_entry *)CQ_entry_addr;

		u32 addr_data = Xil_In32(CQ_entry_addr);

		//		if (cqpolling%50000 == 0) {
		//			xil_printf("%x, %x\n",addr_data,my_polling_status[i].polling_phase);
		//		}

		if ((addr_data & 0x0C000000) >> 26 == my_polling_status[i].polling_phase)
		{
			//			xil_printf("my loop test \n");

			if (my_polling_status[0].complete_num % 10000 == 0)
			{
				xil_printf("[INFO] completed page number is %d, \n", my_polling_status[0].complete_num);
				// XTime_GetTime(&tmp_test_end_time);
				  // xil_printf("[INFO] completed page number is %d, bandwidth is %d MB/s \n", \
						my_polling_status[0].complete_num, \
						160000000/1024/((tmp_test_end_time - tmp_test_start_time)/400000));
				// XTime_GetTime(&tmp_test_start_time);
			}
			my_polling_status[0].complete_num++;

			my_polling_status[i].polling_index++;
			if (my_polling_status[i].polling_index == 256)
			{
				my_polling_status[i].polling_index = 0;
				my_polling_status[i].polling_phase++;
				my_polling_status[i].polling_phase = my_polling_status[i].polling_phase % 4;
			}
			u8 SQ_index = CQ_entry->cur_ptr;

			hw_queue_entry *SQ_entry;
			switch (i)
			{
			case 0:
				SQ_entry = (hw_queue_entry *)(HW_CH0_SQ_ENTRY_ADDR + SQ_index * 16);
				break;
			case 1:
				SQ_entry = (hw_queue_entry *)(HW_CH1_SQ_ENTRY_ADDR + SQ_index * 16);
				break;
			case 2:
				SQ_entry = (hw_queue_entry *)(HW_CH2_SQ_ENTRY_ADDR + SQ_index * 16);
				break;
			case 3:
				SQ_entry = (hw_queue_entry *)(HW_CH3_SQ_ENTRY_ADDR + SQ_index * 16);
				break;
			default:
				SQ_entry = 0; //
			}

			u32 hcmd_entry_index = SQ_entry->hcmd_index;
			emu_log_println(LOG, "get hcmd  %u in cqpolling", hcmd_entry_index);
			FCL_free_SQ_entry(i, SQ_index);
			// release_spin_lock((u32 *)SYN_ERASE_SPIN_LOCK);

			//			xil_printf("core1, hcmd_entry_index : %d\n",hcmd_entry_index);
			//			xil_printf("core1, opc : %d\n",CQ_entry->op);

			//			if (hcmd_entry_index != 0xff) { //hcmd_entry_index != 0xff
			////				xil_printf("%d \n",CQ_entry->op);
			//				if (CQ_entry->op == HCE_ERASE) {
			//					release_spin_lock((u32 *)SYN_ERASE_SPIN_LOCK);
			//				} else {
			//
			//				}
			//			}

#if 1

			if (my_polling_status[0].complete_num % 50000 == 0)
			{
				xil_printf("CQ OPC = %d, cmd space = %d, hcmd sp = %d, gmcd sp = %d \n", CQ_entry->op, CQ_entry->cmd_space, HCMD_SPACE, GCMD_SPACE);
			}

			if (CQ_entry->cmd_space == HCMD_SPACE)
			{
				if (CQ_entry->op == HCE_ERASE)
				{
					release_spin_lock((u32 *)SYN_ERASE_SPIN_LOCK);
				}
				else
				{
					host_cmd_entry *hcmd_entry = HCL_get_hcmd_entry_addr(hcmd_entry_index);
					hcmd_entry->cpl_cnt++;
					if (hcmd_entry->cpl_cnt == hcmd_entry->req_num)
					{
						get_spin_lock((u32 *)TQ_FROMCQ_SPIN_LOCK);
						//						host_cmd_entry* hcmd_entry = HCL_get_hcmd_entry_addr(hcmd_entry_index);
						FTL_sendhcmd(hcmd_entry, HCE_FROM_CQ);
						release_spin_lock((u32 *)TQ_FROMCQ_SPIN_LOCK);
					}
				}
			}
			else
			{
				gc_command_entry *hcmd_entry = gcmd_entry;
				//				xil_printf("gcmd opc = %d, gc read = %d, gc write = %d\n",hcmd_entry->op_code,GCE_READ,GCE_WRITE);

				if (hcmd_entry->op_code == GCE_READ)
				{
					hcmd_entry->read_cpl_cnt++;
					//					xil_printf("cq polling : %d/%d\n",hcmd_entry->read_cpl_cnt,hcmd_entry->gc_valid_page_index);
					if (hcmd_entry->read_cpl_cnt >= hcmd_entry->gc_valid_page_index)
					{
						// hcmd_entry->op_code == GCE_READ;
						//						GC_print_gcmd("cq polling",hcmd_entry);
						hcmd_entry->status = GCE_GC_FROM_CQ;
					}
				}
				else if (hcmd_entry->op_code == GCE_WRITE)
				{
					hcmd_entry->write_cpl_cnt++;
					if (hcmd_entry->write_cpl_cnt == hcmd_entry->gc_valid_page_index)
					{
						hcmd_entry->status = GCE_GC_FROM_CQ;
					}
				}
			}
#endif
		}
	}
#endif
}
