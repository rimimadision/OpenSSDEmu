#ifndef L2P_H
#define L2P_H

#include "../lib/type.h"
#include "../fcl/fcl.h"
#include "../config/config.h"
#include "../config/mem.h"
#include "../emu/emu_log.h"

/*
 *  MACRO
 */
/* SUBPAGE */
#define SUBPAGE_PER_PAGE 4
#define SUB_PAGE_SZ SUB_PAGE_SIZE
#define SUB_PAGE_NUM (u32)(SSD_SZ / SUB_PAGE_SZ)

/*
 * structure definition of L2P
 */
typedef union _psa_entry
{
    struct
    {
        u32 subpage : 2;
        u32 page : 11;  // 1152 pages per block
        u32 block : 10; // 1006 blocks per plane
        u32 plane : 1;  // 2 planes per lun
        u32 lun : 1;    // lun
        u32 ce : 3;
        u32 ch : 2;
        u32 dirty : 1;
        u32 valid : 1;

    } tlc_ppa;
    struct
    {
        u32 subpage : 2;
        u32 plane : 1;  // 2 planes per lun
        u32 page : 9;   // 384 pages per block
        u32 block : 10; // 1006 blocks per plane
        u32 lun : 3;    // 1 lun
        u32 ce : 3;
        u32 ch : 2;
        u32 dirty : 1;
        u32 valid : 1;

    } slc_psa;
    u32 addr;
} psa_entry;

typedef struct _l2p_table_entry
{
    psa_entry psa;
} l2p_table_entry;

typedef union _global_table_entry
{
    struct
    {
        u32 page : 9;   // 384 pages per block
        u32 block : 10; // 1006 blocks per plane
        u32 plane : 1;  // 2 planes per lun
        u32 lun : 3;    // 1 lun
        u32 ce : 3;
        u32 ch : 2;
        u32 dirty : 1;
        u32 resv : 3;
    } ppa;
    u32 addr;
} global_table_entry;

typedef struct _block_entry
{
    u8 free[PAGE_PER_BLOCK / 8 + 1][PAGE_SIZE / SUB_PAGE_SZ];
    u8 invalid[PAGE_PER_BLOCK / 8 + 1][PAGE_SIZE / SUB_PAGE_SZ];
    u16 invalid_cnt;
    u16 valid_cnt;
} block_entry;

/*
 *  MACRO
 */
/* L2P_TABLE */
typedef u32 l2p_table_index;
#define L2P_TABLE_LEN SUB_PAGE_NUM
#define MIN_LSN (0)
#define MAX_LSN (L2P_TABLE_LEN - 1)
#define L2P_TABLE_SZ (sizeof(l2p_table_entry) * L2P_TABLE_LEN)
#define L2P_TABLE_PAGE_NUM DIV_ROUND_UP(L2P_TABLE_SZ, FLASH_PAGE_SZ)
#define L2P_ENTRY_INVALID_BIT (0b0)
#define L2P_ENTRY_VALID_BIT (0b1)
#define L2P_MASK (~(0b0011 + (0b1100 << 28))) // it mask psa_entry without subpage dirty valid
static_assert(L2P_TABLE_SZ < L2P_TABLE_SIZE_LIMIT); // TODO: mem_map assign the max_sz of l2p_table

/* G_TABLE */
#define G_TABLE_LEN L2P_TABLE_PAGE_NUM
#define G_TABLE_SZ (sizeof(global_table_entry) * G_TABLE_LEN)
#define G_TABLE_PAGE_NUM DIV_ROUND_UP(G_TABLE_SZ / FLASH_PAGE_SZ)
#define G_TABLE_LEN_PER_PAGE (FLASH_PAGE_SZ / sizeof(global_table_entry))
/* TODO: static assert of g_table for out of bounds */

/* BLOCK_TABLE */
#define BIT u8 // can only be SET or RESET
#define SET (0b1)
#define RESET (0b0)
#define set_bit(x, n)  \
    do                 \
    {                  \
        x |= (1 << n); \
    } while (0)
#define reset_bit(x, n) \
    do                  \
    {                   \
        x &= ~(1 << n); \
    } while (0)
typedef enum bitmap_t
{
    FREE_BITMAP = 0,
    INVALID_BITMAP
} bitmap_t;
#define BLOCK_TABLE_NUM TOTAL_BLOCK
#define BLOCK_TABLE_LEN BLOCK_TABLE_NUM
#define BLOCK_TABLE_SZ (sizeof(block_entry) * BLOCK_TABLE_NUM)
static_assert(BLOCK_TABLE_SZ < (30 * MB), "block table out of bound");
#define block_table_entry(ch, ce, lun, plane, block) (*(block_table + ch * BLOCK_PER_CH + ce * BLOCK_PER_CE + lun * BLOCK_PER_LUN + plane * BLOCK_PER_PLANE + block))

/*
 *  global variables
 */
extern block_entry *block_table; // 用来记录block中page的的状态，指向一个4维数组的首地址，
                                 // 这个这个4维数组在L2P_init_l2p_module()函数中进行地址划分和初始化，
                                 // 访问数组元素的形式可为block_table[ch][ce][lun][plane][block]

extern global_table_entry *g_table; //用来记录global table，指向一个1维数组的首地址

extern l2p_table_entry *l2p_table; //用来记录l2p table，指向一个1维数组的首地址

/* 
 * Static function
 */
/* Assign value for psa */
static inline void L2P_set_psa(psa_entry *e,
							   u32 ch,
							   u32 ce,
							   u32 lun,
							   u32 plane,
							   u32 block,
							   u32 page,
							   u32 subpage)
{
#if (FLASH_TYPE == SLC)
	assert(ch < TOTAL_CH);
	assert(ce < CE_PER_CH);
	assert(lun < LUN_PER_CE);
	assert(plane < PLANE_PER_LUN);
	assert(block < BLOCK_PER_PLANE);
	assert(page < PAGE_PER_BLOCK);
	assert(subpage < SUBPAGE_PER_PAGE);

	e->slc_psa.ch = ch;
	e->slc_psa.ce = ce;
	e->slc_psa.lun = lun;
	e->slc_psa.plane = plane;
	e->slc_psa.block = block;
	e->slc_psa.page = page;
	e->slc_psa.subpage = subpage;
#endif
}
/* print psa */
static inline void L2P_print_psa(psa_entry *e)
{
#if (FLASH_TYPE == SLC)
	EMU_log_println(LOG, "psa : %u %u %u %u %u %u %u",
					e->slc_psa.ch,
					e->slc_psa.ce,
					e->slc_psa.lun,
					e->slc_psa.plane,
					e->slc_psa.block,
					e->slc_psa.page,
					e->slc_psa.subpage);
#endif
}

/*
 * API of L2P
 */
/*初始化L2P模块，对L2P使用的全局变量进行初始化，调用L2P_init_global_table和L2P_init_l2p_table*/
void L2P_init_l2p_module();
/* 根据lsn获得一个l2p entry */
l2p_table_entry L2P_get_l2p_entry(l2p_table_index lsn, u32 opcode, u32 is_searched);
/*根据lsn获得其l2p entry所在的地址，即global entry*/
global_table_entry L2P_get_global_table_entry(l2p_table_index lsn);
/*向flash下刷有更新l2p entry的page*/
void L2P_update_l2p_table_2(); // ISSUE: 名字重复
/*向flash下刷有更新global entry的page*/
void L2P_update_global_table();
/* for unit test of L2P */
void L2P_unit_test();

extern u32 *lpn_ppn;
extern u8 *ppn_state;
void L2P_calc_obj_ppa(u32 obj_id, u64 obj_offset, phy_page_addr *ppa);
void L2P_calc_ppa(u32 lpn, phy_page_addr *ppa);
void FTL_erase_init(void);

#endif
