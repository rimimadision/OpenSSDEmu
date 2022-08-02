#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "l2p.h"
#include "../config/config.h"
#include "../config/mem.h"
#include "../fcl/fcl.h"
#include "../gc/gc.h"
#include "../ftl/ftl_core0.h"
#include "../lib/lock.h"
#include "../lib/assert.h"

#include "../emu/emu_log.h"

/*
 * global variable of L2P
 */

psa_entry global_psa; // 记录下一个待分配地址 // NOTE: 修改文档

block_entry *block_table; // 用来记录block中page的的状态，指向一个4维数组的首地址，
						  // 这个这个4维数组在L2P_init_l2p_module()函数中进行地址划分和初始化，
						  // 访问数组元素的形式可为block_table[ch][ce][lun][plane]
						  // NOTE: 修改文档

global_table_entry *g_table; //用来记录global table，指向一个1维数组的首地址

l2p_table_entry *l2p_table; //用来记录l2p table，指向一个1维数组的首地址

/*
 * internal functions declaration of L2P
 */
static void L2P_init_l2p_table();
static void L2P_init_global_table();
static boolean L2P_search_l2p_entry(l2p_table_index lsn, l2p_table_entry *l2p_entry);
static void L2P_update_l2p_table(l2p_table_index lsn, l2p_table_entry l2p_entry);
static psa_entry L2P_get_new_psa();
static psa_entry L2P_get_and_update_global_psa();

/*
 * inline functions of L2P
 */

/* make l2p entry invalid */
static inline void L2P_invalid_l2p_entry(l2p_table_index lsn)
{
#if (FLASH_TYPE == SLC)
	l2p_table[lsn].psa.slc_psa.valid = L2P_ENTRY_INVALID_BIT;
#endif
}

/* make l2p entry valid */
static inline void L2P_valid_l2p_entry(l2p_table_index lsn)
{
#if (FLASH_TYPE == SLC)
	l2p_table[lsn].psa.slc_psa.valid = L2P_ENTRY_VALID_BIT;
#endif
}

/* if l2p entry valid */
static inline boolean L2P_if_l2p_entry_valid(l2p_table_index lsn)
{
#if (FLASH_TYPE == SLC)
	return (l2p_table[lsn].psa.slc_psa.valid == L2P_ENTRY_VALID_BIT);
#endif
}

/* if two psa same */
static inline boolean L2P_if_psa_equal(psa_entry *a, psa_entry *b)
{
#if (FLASH_TYPE == SLC)
	return (a->slc_psa.ch == b->slc_psa.ch &&
			a->slc_psa.ce == b->slc_psa.ce &&
			a->slc_psa.lun == a->slc_psa.lun &&
			a->slc_psa.plane == b->slc_psa.plane &&
			a->slc_psa.block == b->slc_psa.block &&
			a->slc_psa.page == b->slc_psa.page &&
			a->slc_psa.subpage == b->slc_psa.subpage);
#endif
}

/* update psa with specific order, if reachs end, then return FALSE */
static inline boolean L2P_inc_psa(psa_entry *psa)
{
#if (FLASH_TYPE == SLC)
	if (psa->slc_psa.subpage == SUBPAGE_PER_PAGE - 1)
	{
		/* start from begining of subpages */
		psa->slc_psa.subpage = 0;
		/*
		 * Subpages of current page run out,
		 * then we need to switch to a new page
		 */

		/*
		 * here is our increament order to find new page :
		 * plane->ch->ce->lun->page->block
		 * in that case we can have higer concurrency
		 */
		if (psa->slc_psa.plane == PLANE_PER_LUN - 1)
		{
			psa->slc_psa.plane = 0;
			if (psa->slc_psa.ch == TOTAL_CH - 1)
			{
				psa->slc_psa.ch = 0;
				if (psa->slc_psa.ce == CE_PER_CH - 1)
				{
					psa->slc_psa.ce = 0;
					if (psa->slc_psa.lun == LUN_PER_CE - 1)
					{
						psa->slc_psa.lun = 0;
						if (psa->slc_psa.page == PAGE_PER_BLOCK - 1)
						{
							psa->slc_psa.page = 0;
							if (psa->slc_psa.block == BLOCK_PER_PLANE - 1)
							{
								/*
								 * We can't find available subpage after global_psa
								 */
								return FALSE;
							}
							else
							{
								psa->slc_psa.block++;
							}
						}
						else
						{
							psa->slc_psa.page++;
						}
					}
					else
					{
						psa->slc_psa.lun++;
					}
				}
				else
				{
					psa->slc_psa.ce++;
				}
			}
			else
			{
				psa->slc_psa.ch++;
			}
		}
		else
		{
			psa->slc_psa.plane++;
		}
	}
	else
	{
		/* if we still get subpage not try */
		psa->slc_psa.subpage++;
	}

	return TRUE;
#endif
}

/* get subpage status from bitmap in block_table */
static inline BIT L2P_get_subpage_status(bitmap_t T, psa_entry psa)
{
#if (FLASH_TYPE == SLC)
	if (T == FREE_BITMAP)
	{
		return ((block_table_entry(psa.slc_psa.ch,
								   psa.slc_psa.ce,
								   psa.slc_psa.lun,
								   psa.slc_psa.plane,
								   psa.slc_psa.block)
					 .free[psa.slc_psa.page / 8]
						  [psa.slc_psa.subpage] >>
				 (psa.slc_psa.page % 8)) &
				(1 << 0));
	}
	else if (T == INVALID_BITMAP)
	{
		return ((block_table_entry(psa.slc_psa.ch,
								   psa.slc_psa.ce,
								   psa.slc_psa.lun,
								   psa.slc_psa.plane,
								   psa.slc_psa.block)
					 .invalid[psa.slc_psa.page / 8]
							 [psa.slc_psa.subpage] >>
				 (psa.slc_psa.page % 8)) &
				(1 << 0));
	}
#endif
}

/* set subpage status in bitmap of block_table */
static inline void L2P_set_subpage_status(bitmap_t T, psa_entry *psa, BIT bit)
{
#if (FLASH_TYPE == SLC)
	if (T == FREE_BITMAP)
	{
		if (SET == bit)
		{
			set_bit(block_table_entry(psa->slc_psa.ch,
									  psa->slc_psa.ce,
									  psa->slc_psa.lun,
									  psa->slc_psa.plane,
									  psa->slc_psa.block)
						.free[psa->slc_psa.page / 8][psa->slc_psa.subpage],
					(psa->slc_psa.page % 8));
		}
		else if (RESET == bit)
		{
			reset_bit(block_table_entry(psa->slc_psa.ch,
										psa->slc_psa.ce,
										psa->slc_psa.lun,
										psa->slc_psa.plane,
										psa->slc_psa.block)
						  .free[psa->slc_psa.page / 8][psa->slc_psa.subpage],
					  (psa->slc_psa.page % 8));
		}
	}
	else if (T == INVALID_BITMAP)
	{
		if (SET == bit)
		{
			set_bit(block_table_entry(psa->slc_psa.ch,
									  psa->slc_psa.ce,
									  psa->slc_psa.lun,
									  psa->slc_psa.plane,
									  psa->slc_psa.block)
						.invalid[psa->slc_psa.page / 8][psa->slc_psa.subpage],
					(psa->slc_psa.page % 8));
		}
		else if (RESET == bit)
		{
			reset_bit(block_table_entry(psa->slc_psa.ch,
										psa->slc_psa.ce,
										psa->slc_psa.lun,
										psa->slc_psa.plane,
										psa->slc_psa.block)
						  .invalid[psa->slc_psa.page / 8][psa->slc_psa.subpage],
					  (psa->slc_psa.page % 8));
		}
	}
#endif
}

/*
 * internal functions of L2P
 */

static void L2P_init_block_table()
{
	u32 ch, ce, lun, plane, block, i, subpage;
	block_table = (block_entry *)(L2P_BASE_ADDR);
	EMU_log_println(LOG, "block table start from %x", (u32)block_table);

	/* TODO: read block_table from flash */

	/* Set valid_cnt and invalid_cnt */
	memset((void *)block_table, 0, BLOCK_TABLE_SZ);

	for (ch = 0; ch < TOTAL_CH; ch++)
	{
		for (ce = 0; ce < CE_PER_CH; ce++)
		{
			for (lun = 0; lun < LUN_PER_CE; lun++)
			{
				for (plane = 0; plane < PLANE_PER_LUN; plane++)

				{
					for (block = 0; block < BLOCK_PER_PLANE; block++)
					{
						for (i = 0; i < PAGE_PER_BLOCK / 8; i++)
						{
							for (subpage = 0; subpage < PAGE_SIZE / SUB_PAGE_SZ; subpage++)
							{
								block_table_entry(ch, ce, lun, plane, block).free[i][subpage] = 0xff;
							}
						}
					}
				}
			}
		}
	}
	/* NOTE: Set all block to free for now */
#if 0
	psa_entry new_psa;
	L2P_set_psa(&new_psa, 0, 0, 0, 0, 0, 0, 0);
	for (;;)
	{
		L2P_set_subpage_status(FREE_BITMAP, &new_psa, SET);
		L2P_set_subpage_status(INVALID_BITMAP, &new_psa, RESET);
		if (FALSE == L2P_inc_psa(&new_psa))
		{
			break;
		}
	}
#endif
}

/*给l2p_table指针分配地址，初始化l2p_table*/
static void L2P_init_l2p_table()
{
	l2p_table = (l2p_table_entry *)(L2P_BASE_ADDR + BLOCK_TABLE_SZ);
	u64 b = (u64)1024 * 1024 * 1024 * 128 / 4096;
	u64 i;
	for (i = 0; i < L2P_TABLE_LEN; i++)
	{
		l2p_table[i].psa.addr = 0;
		//L2P_invalid_l2p_entry(i);
	}

	EMU_log_println(LOG, "l2p entry  0x%x  \n", l2p_table);

	EMU_log_println(LOG, "b 0x%llx,L2P_TABLE_LEN = 0x%llx\n ", b, L2P_TABLE_LEN);
}

/*初始化global_table指针分配地址，初始化global_table*/
static void L2P_init_global_table()
{
	g_table = (global_table_entry *)(L2P_BASE_ADDR + BLOCK_TABLE_SZ + L2P_TABLE_SZ);
	/* TODO: read g_table from flash */
}

/*根据lsn查找相应的l2p entry,并获得l2p_entry内部的valid,此函数是提供模块内部的API */
static boolean L2P_search_l2p_entry(l2p_table_index lsn, l2p_table_entry *l2p_entry)
{
	if (FALSE == L2P_if_l2p_entry_valid(lsn))
	{
		return FALSE;
	}

	memcpy(l2p_entry, &l2p_table[lsn], sizeof(l2p_table_entry));
	return TRUE;
}

/*修改lsn对应的l2p entry，此函数是提供模块内部的API*/
static void L2P_update_l2p_table(l2p_table_index lsn, l2p_table_entry l2p_entry)
{
	l2p_table[lsn] = l2p_entry;
	L2P_valid_l2p_entry(lsn);
}

/* alloc new global psa from start */
static psa_entry L2P_get_new_psa()
{
	/* start from begining of psa */
	psa_entry new_psa;

	for (new_psa.addr = 0;
		 FALSE == L2P_if_psa_equal(&new_psa, &global_psa);
		 L2P_inc_psa(&new_psa))
	{
		if (SET == L2P_get_subpage_status(FREE_BITMAP, new_psa))
		{
			/* this subpage is available, set it RESET as occupied */
			L2P_set_subpage_status(FREE_BITMAP, &new_psa, RESET);
			return new_psa;
		}
	}

	assert(!("L2P_get_new_psa get new psa reachs global psa!!!"));
}

static psa_entry L2P_get_and_update_global_psa()
{
#if (FLASH_TYPE == SLC)
	/*
	 * find subpage may be available after global_psa
	 * by increament psa by specific order
	 * Start from where we alloc psa before
	 */
	psa_entry new_psa;
	u32 ret;
	static int first_init = 0;
	if ((first_init == 0) && (global_psa.addr == 0))
	{
		new_psa = global_psa;
		first_init = 1;
		if (SET == L2P_get_subpage_status(FREE_BITMAP, new_psa))
		{
			/* this subpage is available, set it RESET as occupied */
			L2P_set_subpage_status(FREE_BITMAP, &new_psa, RESET);
			return new_psa;
		}
	}

	while (1) //??
	{
		/*
		 * find available subpage in page
		 */

		ret = L2P_inc_psa(&global_psa);
		if (ret == FALSE)
		{
			break;
		}
		new_psa = global_psa;
		if (SET == L2P_get_subpage_status(FREE_BITMAP, new_psa))
		{
			/* this subpage is available, set it RESET as occupied */
			L2P_set_subpage_status(FREE_BITMAP, &new_psa, RESET);
			return new_psa;
		}
	}
	/*
	 * We can't find available subpage after global_psa
	 * so we need to find a new glabal_psa from start
	 */
	global_psa = L2P_get_new_psa();
	new_psa = global_psa;
	return new_psa;
#endif
}

/*
 * API of L2P
 */

/*初始化L2P模块，对L2P使用的全局变量进行初始化，调用L2P_init_global_table和L2P_init_l2p_table */
void L2P_init_l2p_module()
{
	// mutex.lock
	// defer mutex.unlock

	/* initialize global psa */
	memset(&global_psa, 0, sizeof(psa_entry));

	/* initialize block_table */
	L2P_init_block_table();
	L2P_init_global_table();
	L2P_init_l2p_table();
}

/*根据lsn获得一个l2p entry，此函数是提供给别的模块的API*/
l2p_table_entry L2P_get_l2p_entry(l2p_table_index lsn, u32 opcode, u32 is_searched)
{
	// mutex.lock
	// defer mutex.unlock

	assert(lsn <= MAX_LSN);
	l2p_table_entry ret_entry;

	/* When get l2p entry for read_op, so we need to use the former entry */
	if ((opcode == HCE_READ) || (is_searched == TRUE))
	{
		if (TRUE == L2P_search_l2p_entry(lsn, &ret_entry))
		{
			return ret_entry;
		}
		// TODO: if can't find former entry in DRAM, then we need to check
		// if it's in FLASH

		// ....
	}

	/*
	 * When get l2p entry for write_op, then we need to out-place update
	 * make the former l2p_entry invalid and get a new entry
	 */

	ret_entry.psa = L2P_get_and_update_global_psa();

	L2P_update_l2p_table(lsn, ret_entry);
	L2P_update_global_table(); // TODO:

	return ret_entry;
}

/*根据lpn获得其l2p entry所在的地址，即global entry*/
global_table_entry L2P_get_global_table_entry(l2p_table_index lsn)
{
	// mutex.lock
	// defer mutex.unlock

	assert((lsn / G_TABLE_LEN_PER_PAGE) < G_TABLE_LEN);
	return g_table[lsn / G_TABLE_LEN_PER_PAGE];
}

/*向flash下刷有更新l2p entry的page*/
void L2P_update_l2p_table_2() // TODO:
{
	// mutex.lock
	// defer mutex.unlock
}

/*向flash下刷有更新global entry的page*/
void L2P_update_global_table() // TODO:
{
	// mutex.lock
	// defer mutex.unlock
}

#ifdef EMU
/* for unit test of L2P */
void L2P_unit_test()
{
	/* test bit ops */
	// block_table = (block_entry *)(MEM_BASE);
	// memset((void *)block_table, 0xff, BLOCK_TABLE_SZ);
	// psa_entry e;
	// L2P_set_psa(&e, 1, 3, 0, 1, 200, 40, 2);
	// EMU_log_println(LOG, "status");
	// // L2P_set_subpage_status(FREE_BITMAP, &e, SET);
	// EMU_log_println(LOG, "status1 : %u", L2P_get_subpage_status(FREE_BITMAP, e));
	// L2P_set_subpage_status(FREE_BITMAP, &e, RESET);
	// EMU_log_println(LOG, "status2 : %u", L2P_get_subpage_status(FREE_BITMAP, e));
	// exit(0);

	/* test L2P_inc_psa */
	// block_table = (block_entry *)(MEM_BASE);
	// EMU_log_println(LOG, "block table start from %u", (u32)block_table);
	// memset((void *)block_table, 0x0, BLOCK_TABLE_SZ);
	// psa_entry new_psa;
	// L2P_set_psa(&new_psa, 0, 0, 0, 0, 0, 0, 0);
	// // L2P_inc_psa(&new_psa);
	// // L2P_print_psa(&new_psa);
	// // exit(0);
	// for (;;)
	// {
	// 	// L2P_print_psa(&new_psa);
	// 	L2P_set_subpage_status(FREE_BITMAP, &new_psa, SET);
	// 	L2P_set_subpage_status(INVALID_BITMAP, &new_psa, RESET);
	// 	if (FALSE == L2P_inc_psa(&new_psa))
	// 	{
	// 		break;
	// 	}
	// }
	// u32 ce;
	// u32 ch;
	// u32 plane;
	// u32 block;
	// u32 page;
	// psa_entry e;
	// // use normal order iteration to check if every psa is accessed
	// for (u32 i = 0; i < TOTAL_CHANNEL; i++)
	// {
	// 	e.slc_psa.ch = i;
	// 	for (u32 j = 0; j < CE_PER_CH; j++)
	// 	{
	// 		e.slc_psa.ce = j;
	// 		for (u32 k = 0; k < LUN_PER_CE; k++)
	// 		{
	// 			e.slc_psa.lun = k;
	// 			for (u32 l = 0; l < PLANE_PER_LUN; l++)
	// 			{
	// 				e.slc_psa.plane = l;
	// 				for (u32 m = 0; m < BLOCK_PER_PLANE; m++)
	// 				{
	// 					e.slc_psa.block = m;
	// 					for (u32 n = 0; n < PAGE_PER_BLOCK; n++)
	// 					{
	// 						e.slc_psa.page = n;
	// 						for (u32 o = 0; o < SUBPAGE_PER_PAGE; o++)
	// 						{
	// 							e.slc_psa.subpage = o;
	// 							if (RESET == L2P_get_subpage_status(FREE_BITMAP, e))
	// 							{
	// 								assert(!"test fault");
	// 							}
	// 						}
	// 					}
	// 				}
	// 			}
	// 		}
	// 	}
	// }
	// EMU_log_println(LOG, "exit");
	// exit(0);

	/* test init fuction */
	L2P_init_l2p_module();

	/* test L2P_get_new_psa */
	L2P_set_psa(&global_psa, 2, 3, 0, 1, 400, 300, 2);
	psa_entry new_psa;
	memcpy(&new_psa, &global_psa, sizeof(psa_entry));
	for (;;)
	{
		L2P_set_subpage_status(FREE_BITMAP, &new_psa, RESET);
		if (FALSE == L2P_inc_psa(&new_psa))
		{
			break;
		}
	}
	L2P_print_psa(&global_psa);
	L2P_get_and_update_global_psa();
	L2P_print_psa(&global_psa);
	while (1)
		;
}

#endif

void L2P_calc_obj_ppa(u32 obj_id, u64 obj_offset, phy_page_addr *ppa)
{
	// u32 circle_num = 0; // (u32)obj_offset;

	// u32 normal_ch[TOTAL_CHANNEL] = NORMAL_CHANNEL;
	// u32 ce;
	// u32 ch;
	// u32 plane;
	// u32 page;

	// for (page = 0; (page < PAGE_PER_BLOCK_SLC_MODE); page++)
	// {
	// 	for (plane = 0; (plane < PLANE_PER_CE); plane++)
	// 	{
	// 		for (ce = 0; (ce < CE_PER_CH); ce++)
	// 		{
	// 			for (ch = 0; (ch < TOTAL_CHANNEL); ch++)
	// 			{
	// 				if (normal_ch[ch] == 1)
	// 				{
	// 					if (circle_num == obj_offset)
	// 					{
	// 						ppa->ch = ch;
	// 						ppa->ce = ce;
	// 						ppa->nand_flash_addr.slc_mode.block = obj_id;
	// 						ppa->nand_flash_addr.slc_mode.lun = 0;
	// 						ppa->nand_flash_addr.slc_mode.plane = plane;
	// 						ppa->nand_flash_addr.slc_mode.page = page;

	// 						// xil_printf("ch %d  ce %d \n", ch, ce);

	// 						return;
	// 					}
	// 					circle_num++;
	// 				}
	// 			}
	// 		}
	// 	}
	// }
}
void L2P_calc_ppa(u32 lpn, phy_page_addr *ppa)
{
	u32 circle_num = 0;
	u32 normal_ch[TOTAL_CHANNEL] = NORMAL_CHANNEL;

	u32 valid_ch_num = 0;
	for (u32 i = 0; i < TOTAL_CHANNEL; i++)
	{
		if (normal_ch[i] == 1)
		{
			valid_ch_num++;
		}
	}

	u32 ce;
	u32 ch;
	u32 plane;

	u32 row = lpn / (valid_ch_num * CE_PER_CH * PLANE_PER_LUN * PAGE_PER_BLOCK_SLC_MODE);
	u32 row_offset = lpn % (valid_ch_num * CE_PER_CH * PLANE_PER_LUN * PAGE_PER_BLOCK_SLC_MODE);
	u32 page = row_offset / (valid_ch_num * CE_PER_CH * PLANE_PER_LUN);
	u32 page_offset = row_offset % (valid_ch_num * CE_PER_CH * PLANE_PER_LUN);

	for (plane = 0; (plane < PLANE_PER_CE); plane++)
	{
		for (ce = 0; (ce < CE_PER_CH); ce++)
		{
			for (ch = 0; (ch < TOTAL_CHANNEL); ch++)
			{
				if (normal_ch[ch] == 1)
				{
					if (circle_num == page_offset)
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
void FTL_erase_init(void)
{
	u32 ch, ce, lun, plane, block;
	u32 sq_id = INVALID_INDEX;
	nand_flash_addr_cmd phyaddr;
	u32 hcmd_id = 0;
	u32 buff_id = 0;
	u32 ret = FAIL;
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

						phyaddr.tlc_mode.lun = lun;
						phyaddr.tlc_mode.plane = plane;
						phyaddr.tlc_mode.block = block;
						// FCL_set_SQ_entry(hcmd_id, sq_id, buff_id, 3, &phyaddr);

						while (ret == FAIL)
						{
							// TO DO ADD TIME OUT CHECK
							ret = FCL_send_SQ_entry(sq_id, ch, ce);
						}
					}
				}
			}
		}
	}
}