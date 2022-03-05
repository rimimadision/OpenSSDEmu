#include"l2p.h"

#include"../config/config.h"
#include"../fcl/fcl.h"
#include"../gc/gc.h"
#include"../ftl/ftl_core0.h"
#include "../emu/emu_log.h"
#include "../lib/lock.h"

#include<stdlib.h>
//#include<_timeval.h>
#include<time.h>

//int (*arrayptr)[5];
u32 *lpn_ppn;
u8 *ppn_state;
//u32 (*lpn_ppn)[1] = GC_LPN_PPN;
//u8 (*ppn_state)[1] = GC_PPN_STATE;
extern u32 tmp_sq_index;
extern u32 used_page;

u32 maxint(u32 a, u32 b) {
	return a > b ? a : b;
}

void L2P_print_ppa(phy_page_addr ppa) {
	xil_printf("===================================\n");
	xil_printf("ch : %d\n",ppa.ch);
	xil_printf("ce : %d\n",ppa.ce);
	xil_printf("lun : %d\n",ppa.nand_flash_addr.slc_mode.lun);
	xil_printf("plane : %d\n",ppa.nand_flash_addr.slc_mode.plane);
	xil_printf("block : %d\n",ppa.nand_flash_addr.slc_mode.block);
	xil_printf("page : %d\n",ppa.nand_flash_addr.slc_mode.page);
	xil_printf("-----------------------------------\n");
}


void L2P_init()
{
	xil_printf("L2P_init start!\n");
//	FTL_erase_init();

//	FTL_syn_erase_ch(0);
//	FTL_syn_erase_ch(1);
//	FTL_syn_erase_ch(2);
//	FTL_syn_erase_ch(3);

//	while(1);

	u32 maxblock = 0;
	u32 maxipn = 0;

	u32 rand_seed;
	#ifndef EMU
	XTime_GetTime(&rand_seed);
	#else
	rand_seed = time(NULL);
	#endif
	srand((unsigned)rand_seed);
	lpn_ppn = (u32*)GC_LPN_PPN;
	ppn_state = (u8*)GC_PPN_STATE;
	phy_page_addr ppa;

//	xil_printf("l2p init 1\n");

//	memset(lpn_ppn,0,sizeof(u32)*TOTAL_PAGE);
//	memset(ppn_state,0,sizeof(u8)*TOTAL_PAGE);

//	xil_printf("l2p init 2\n");

//	for (u32 i = 0; i< TOTAL_PAGE; i++) {
////		xil_printf("lpn-ppn %d\n",i);
//		if (i % 1000000 == 0) {
//			xil_printf("lpn-ppn : %d\n",i);
//		}
//		lpn_ppn[i] = -1;
//		ppn_state[i] = F;
//	}

	for (u32 ch = 0; ch < CH_PER_BOARD; ch++) {
		// u32 lim_page = PAGE_PER_CHANNEL;
		for (u32 i = 0; i < PAGE_PER_CH; i++) {
			u32 ppn_index = GC_cal_ppn_index(ch,0,i);
			L2P_calc_ppa(ppn_index, &ppa);
//			L2P_print_ppa(ppa);
			u32 ch_no = ch;
			u32 block_no = i/PAGE_PER_BLOCK;

			if (block_no<30000) {
				if (rand()%10<5) {
					lpn_ppn[ppn_index] = ppn_index;
					ppn_state[ppn_index] = I;
					block_list_ptr->block_enrty[ch][block_no].invalid_pages_num++;
					block_list_ptr->block_enrty[ch][block_no].current_page++;
				} else {
					lpn_ppn[ppn_index] = ppn_index;
					ppn_state[ppn_index] = V;
					block_list_ptr->block_enrty[ch][block_no].current_page++;
				}
			}else
			{
				ppn_state[ppn_index] = F;
				lpn_ppn[ppn_index] = -1;
			}


//			lpn_ppn[ppn_index] = ppn_index;

			if (ppn_index % 1000000 == 0) {
				 xil_printf("now page no : %d, now block no : %d, maxblock : %d, maxipn : %d\n",ppn_index,block_no,maxblock,maxipn);
			}

		}
	}


//	for (u32 ch = 0; ch < CH_PER_BOARD; ch++) {
//		// u32 lim_page = PAGE_PER_CHANNEL;
//		for (u32 i = 0; i < BLOCK_PER_CH; i++) {
//			xil_printf("ch = %d, block = %d, curpage = %d, ipn = %d\n",ch,i,block_list_ptr->block_enrty[ch][i].current_page,block_list_ptr->block_enrty[ch][i].invalid_pages_num);
//		}
//	}

	xil_printf("L2P_init end!\n");
}


void L2P_calc_obj_ppa(u32 obj_id, u64 obj_offset ,phy_page_addr * ppa)
{
	u32 circle_num = 0;// (u32)obj_offset;

	u32 normal_ch[TOTAL_CHANNEL] = NORMAL_CHANNEL;
	u32 ce;
	u32 ch;
	u32 plane;
	u32 page;

	for(page = 0; (page < PAGE_PER_BLOCK_SLC_MODE); page++)
	{
		for(plane = 0; (plane < PLANE_PER_CE); plane ++)
		{
			for(ce = 0; (ce < CE_PER_CH); ce++)
			{
				for(ch = 0; (ch < TOTAL_CHANNEL); ch++)
				{
					if(normal_ch[ch] == 1)
					{
						if(circle_num == obj_offset)
						{
							ppa->ch = ch;
							ppa->ce = ce;
							ppa->nand_flash_addr.slc_mode.block = obj_id;
							ppa->nand_flash_addr.slc_mode.lun = 0;
							ppa->nand_flash_addr.slc_mode.plane = plane;
							ppa->nand_flash_addr.slc_mode.page = page;

							// xil_printf("ch %d  ce %d \n", ch, ce);

							return;
						}
						circle_num++;
					}

				}
			}
		}
	}
	return ;
}


void L2P_calc_ppa_bak(u32 lpn, phy_page_addr *ppa)
{
	u32 circle_num = 0;
	u32 normal_ch[TOTAL_CHANNEL] = NORMAL_CHANNEL;

	u32 valid_ch_num = 0;
	for(u32 i = 0; i < TOTAL_CHANNEL; i++)
	{
		if(normal_ch[i] == 1)
		{
			valid_ch_num++;
		}
	}

	u32 ce;
	u32 ch;
	u32 plane;

	u32 row = lpn/(valid_ch_num*CE_PER_CH*PLANE_PER_LUN*PAGE_PER_BLOCK_SLC_MODE);
	u32 row_offset = lpn%(valid_ch_num*CE_PER_CH*PLANE_PER_LUN*PAGE_PER_BLOCK_SLC_MODE);
	u32 page = row_offset/(valid_ch_num*CE_PER_CH*PLANE_PER_LUN);
	u32 page_offset = row_offset%(valid_ch_num*CE_PER_CH*PLANE_PER_LUN);

	for(plane = 0; (plane < PLANE_PER_CE); plane ++)
	{
		for(ce = 0; (ce < CE_PER_CH); ce++)
		{
			for(ch = 0; (ch < TOTAL_CHANNEL); ch++)
			{
				if(normal_ch[ch] == 1)
				{
					if(circle_num == page_offset)
					{
						ppa->ch = ch;
						ppa->ce = ce;
						ppa->nand_flash_addr.slc_mode.block = row;
						ppa->nand_flash_addr.slc_mode.lun = 0;
						ppa->nand_flash_addr.slc_mode.plane = plane;
						ppa->nand_flash_addr.slc_mode.page = page;

						return;
					}
					circle_num++;
				}

				}
			}
	}

}

void L2P_calc_ppa(u32 ppn, phy_page_addr *ppa) {
	// u32 block_num = lpn/PAGE_PER_BLOCK;
	u32 ch = ppn/PAGE_PER_CH;
	u32 ce = ppn%PAGE_PER_CH/PAGE_PER_CE;
	u32 lun = ppn%PAGE_PER_CE/PAGE_PER_LUN;
	u32 plane = ppn%PAGE_PER_CE/PAGE_PER_PLANE;
	u32 block = ppn%PAGE_PER_PLANE/PAGE_PER_BLOCK;
	u32 page = ppn%PAGE_PER_BLOCK;

	ppa->ch = ch;
	ppa->ce = ce;
	ppa->nand_flash_addr.slc_mode.block = block;
	ppa->nand_flash_addr.slc_mode.lun = lun;
	ppa->nand_flash_addr.slc_mode.plane = plane;
	ppa->nand_flash_addr.slc_mode.page = page;
}

void FTL_syn_erase_ch(u32 ch)
{
    phy_page_addr ppa;
    ppa.ch=ch;
    ppa.ce=0;
    ppa.nand_flash_addr.slc_mode.block = 0;
    ppa.nand_flash_addr.slc_mode.plane = 0;

    xil_printf("ftl_syn_erase_ch {%dch} start!\r\n",ch);

	for(u32 ce=0; ce<8; ce++)
	{
		for(u32 plane = 0; plane < 2; plane++)
		{
    		for(u32 block = 0; block < 1006; block++)
    		{
    	        ppa.ch=ch;
    	        ppa.ce=ce;
    	        ppa.nand_flash_addr.slc_mode.block = block;
    	        ppa.nand_flash_addr.slc_mode.plane = plane;

//    	        static u32 tmp_sq_index = 0; //FCL_get_free_SQ_entry(ch);
//    	        tmp_sq_index++;
//    	        tmp_sq_index = tmp_sq_index%256;
    	        //tmp_sq_index = FCL_get_free_SQ_entry(ch);

#if (CORE_MODE == SINGLE_CORE_MODE)
				tmp_sq_index++;
				tmp_sq_index = tmp_sq_index % 256;
#else
        		tmp_sq_index =  FCL_get_free_SQ_entry(ppa.ch);
#endif

    	        host_cmd_entry *hcmd = HCL_get_host_cmd_entry();
				hcmd->op_code = HCE_ERASE;
    	        u32 hcmd_id = HCL_get_hcmd_entry_index(hcmd);
    	        FCL_set_SQ_entry(hcmd_id, tmp_sq_index, 0, HCE_ERASE, &ppa, HCMD_SPACE);

    	        u32 ret = SUCCESS;
    	        do
    	        {
    	        	ret = FCL_send_SQ_entry(tmp_sq_index, ppa.ch, ppa.ce);
    	        }while(ret == FAIL);

//				xil_printf("erase block : %d, %d, %d\n",ch,ce,plane);

                get_spin_lock((u32 *)SYN_ERASE_SPIN_LOCK);

    	        get_spin_lock((u32 *)SYN_ERASE_SPIN_LOCK);
    	        release_spin_lock((u32 *)SYN_ERASE_SPIN_LOCK);
    	        HCL_reclaim_hcmd_entry(hcmd);
    		}
		}
	}

	xil_printf("ftl_syn_erase_ch {%dch} end!\r\n",ch);
}


void FTL_syn_erase_block(u32 ppn_index) {
    phy_page_addr ppa;
//    phy_page_addr ppa;
    L2P_calc_ppa(ppn_index, &ppa);
//    ppa.ch=ch;
//    ppa.ce=ce;
//    ppa.nand_flash_addr.slc_mode.block = block;
//    ppa.nand_flash_addr.slc_mode.plane = plane;

    u32 ch = ppa.ch;

//    u32 tmp_sq_index = FCL_get_free_SQ_entry(ch);

#if (CORE_MODE == SINGLE_CORE_MODE)
	tmp_sq_index++;
	tmp_sq_index = tmp_sq_index % 256;
#else
	tmp_sq_index =  FCL_get_free_SQ_entry(ppa.ch);
#endif

    host_cmd_entry *hcmd = HCL_get_host_cmd_entry();
    hcmd->op_code = HCE_ERASE;
    u32 hcmd_id = HCL_get_hcmd_entry_index(hcmd);
    FCL_set_SQ_entry(hcmd_id, tmp_sq_index, 0, HCE_ERASE, &ppa, HCMD_SPACE);

    u32 ret = SUCCESS;
    do
    {
        ret = FCL_send_SQ_entry(tmp_sq_index, ppa.ch, ppa.ce);
    }while(ret == FAIL);

    get_spin_lock((u32 *)SYN_ERASE_SPIN_LOCK);

    get_spin_lock((u32 *)SYN_ERASE_SPIN_LOCK);
    release_spin_lock((u32 *)SYN_ERASE_SPIN_LOCK);
    HCL_reclaim_hcmd_entry(hcmd);
}


void FTL_erase_init(void)
{
    u32 ch, ce, lun, plane, block;
    u32 sq_id = INVALID_INDEX;
    phy_page_addr phyaddr;
    u32 hcmd_id = 0;
    u32 buff_id = 0;

    memset(&phyaddr, 0, sizeof(nand_flash_addr_cmd));

    for (block = 0; block < BLOCK_PER_PLANE; block++)
    {
        for (lun = 0; lun < LUN_PER_CE; lun++)
        {
            for (plane = 0; plane < PLANE_PER_LUN; plane++)
            {
                for (ce = 0; ce < CE_PER_CH; ce++)
                {
                    for (ch = 0; ch < CH_PER_BOARD; ch++)
                    {

                        while (sq_id == INVALID_INDEX)
                        {
                            sq_id = FCL_get_free_SQ_entry(0);
                        }

                        phyaddr.ch = ch;
                        phyaddr.ce = ce;
                        phyaddr.nand_flash_addr.tlc_mode.lun = lun;
                        phyaddr.nand_flash_addr.tlc_mode.plane = plane;
                        phyaddr.nand_flash_addr.tlc_mode.block = block;
                        FCL_set_SQ_entry(hcmd_id, sq_id, buff_id, 3, &phyaddr, HCMD_SPACE);

                        u32 ret = FAIL;
                        while (ret == FAIL)
                        {
                            //TO DO ADD TIME OUT CHECK
                            ret = FCL_send_SQ_entry(sq_id, ch, ce);
                        }
                    }
                }
            }
        }
    }
}


// void L2P_ppn_items(u32 ppn_index, u32* ch, u32* block) {
// 	u32 ch_div = 8 * 1 * 2 * 1006 * 384;
// 	u32 ch_div = CE_PER_CH * LUN_PER_CE * PLANE_PER_LUN * BLOCK_PER_PLANE * PAGE_PER_BLOCK;
//     u32 block_mod = ch_div / PAGE_PER_BLOCK;

// 	*ch = ppn_index / ch_div;
//     *block = ppn_index / PAGE_PER_BLOCK % block_mod;
// 	return ;
// }


void L2P_search_ppn_gc_v1(u32 *ppn, u32 lpn, hcmd_opcode op_code) 
{
	if (op_code == HCE_READ)
	{
		*ppn = lpn_ppn[lpn];
		return ;
	}
	if (op_code == HCE_WRITE)
	{
		// static u32 ppn_index = 1;

		if (ppn_state[lpn_ppn[lpn]] != F)
		{
			ppn_state[lpn_ppn[lpn]] = I;
			// *ppn_ppa_scheme[*lpn_ppn_scheme[lpn]] = I;
			phy_page_addr ppa;
			L2P_calc_ppa(lpn_ppn[lpn], &ppa);
			// u32 block_no = ppa.ce*CE_PER_CH + ppa.nand_flash_addr.slc_mode.plane*PLANE_PER_CE 
			// 				+ ppa.nand_flash_addr.slc_mode.block;
//			u32 block_no = ppa.nand_flash_addr.slc_mode.block;
//			u32 ch_no = ch;
			u32 block_no = lpn_ppn[lpn]%PAGE_PER_CH/PAGE_PER_BLOCK;
			if (block_list_ptr->block_enrty[ppa.ch][block_no].current_page == PAGE_PER_BLOCK)
			{
//				GC_repair_victim_list(ppa.ch,block_no);
				GC_remove_victim_block(ppa.ch,block_no);
				block_list_ptr->block_enrty[ppa.ch][block_no].invalid_pages_num++;
				GC_add_victim_block(ppa.ch,block_no,block_list_ptr->block_enrty[ppa.ch][block_no].invalid_pages_num);
			}
			else
			{
				block_list_ptr->block_enrty[ppa.ch][block_no].invalid_pages_num++;
				// if (block_list_ptr->block_enrty[ppa.ch][block_no])
			}
		}
		
		for (u32 i = 0; i < CH_PER_BOARD; i++)
		{
			u32 cur_ch_no = GC_get_ch_no();
			channel_entry* cur_ch = &channel_list_ptr->channel_entry[cur_ch_no];

			u32 cur_block_no = cur_ch->current_block;

			// no available block, find next block
//			while (cur_page_no == PAGE_PER_BLOCK)
//			{
//				// GC_remove_free_block(cur_ch_no,cur_block_no);
//				// GC_add_victim_block(cur_ch_no,cur_block_no,cur_block->invalid_pages_num);
////				cur_block = &block_list_ptr->block_enrty[cur_ch_no][] cur_ch->free_block_entry.head_block;
//
//				cur_block_no = cur_ch->current_block;
//				cur_page_no = cur_block->current_page;
//			}

			if (cur_block_no == BLOCK_PTR_NONE)
			{
				continue;
			}

			block_entry* cur_block = &block_list_ptr->block_enrty[cur_ch_no][cur_block_no];
			u32 cur_page_no = cur_block->current_page;

			u32 ppn_index = GC_cal_ppn_index(cur_ch_no,cur_block_no,cur_page_no);
			// ppn_state[lpn] = ppn_index;
			lpn_ppn[lpn] = ppn_index;
			ppn_state[lpn_ppn[lpn]] = V;
			// *ppn_ppa_scheme[ppn_index] = V;
			// cur_block->invalid_pages_num++;
			*ppn = ppn_index;
			cur_block->current_page++;

			if (cur_block->current_page == PAGE_PER_BLOCK)
			{
				GC_remove_free_block(cur_ch_no,cur_block_no);
				GC_add_victim_block(cur_ch_no,cur_block_no,cur_block->invalid_pages_num);
				// GC_remove_free_block(cur_ch_no,cur_block_no);
				cur_ch->current_block = cur_ch->free_block_entry.head_block;
			}
			used_page = used_page+1;
			return ;
		}
	}
	return ;
}
