#ifndef MEM_H
#define MEM_H

#include "../lib/assert.h"
#include "../emu/emu_config.h"
#include "../config/config.h"

static_assert(sizeof(int *) == 4, "should be in 32-bit plantform");

#ifdef EMU
#include "../lib/type.h"
extern u32 mem_base;
extern u32 fcl_base;
#define MEM_BASE mem_base
#define FCL_BASE fcl_base
#else
#define MEM_BASE 0
#endif
// memory map
/*********************************************************************************
// 1GB DRAM
0x00000000->0x0000ffff | 0kb->64kb | size 64kb | OCM | used for
---------------------------------------------------------------
0x00010000->0x0001ffff | 64kb->128kb | size 64kb | OCM | used for hcmd??ask queue
---------------------------------------------------------------
0x00020000->0x0002ffff | 128kb->192kb | size 64kb | OCM | used for
---------------------------------------------------------------
0x00030000->0x000fffff | 192kb->1mb | size 832kb | Dram |
---------------------------------------------------------------
0x00100000->0x003fffff | 1mb->4mb | size 3mb | Dram | used for program??ore0 mmutable
*********************************************************************************/

// ===========================================================================================================
// 0mb->4mb | size 4mb | Dram | used for code, stack, core0 mmu table etc.
// ===========================================================================================================

// ===========================================================================================================
// 4mb->5mb | size 1mb | Dram | used for core1 mmutable. mmutable size is 4096*4=16kb
// ===========================================================================================================
#define CORE1_MMU_TABLE_BASE_ADDR (MEM_BASE + 4 * MB)

// ============================================================================================================
// 5mb->6mb | size 1mb | Dram | used for spin locks
// ============================================================================================================
#define SPIN_LOCK_SZ (sizeof(int))
#define GLOBAL_SPIN_LOCK_1_ADDR (MEM_BASE + 5 * MB) //(770*1024*1024+5*4*1024)
#define CH_SQ_SPIN_LOCK (GLOBAL_SPIN_LOCK_1_ADDR + 1 * SPIN_LOCK_SZ) // only one global spin_lock
#define TQ_FROMCQ_SPIN_LOCK (CH_SQ_SPIN_LOCK + TOTAL_CHANNEL * SPIN_LOCK_SZ)
static_assert((TQ_FROMCQ_SPIN_LOCK + 1 * SPIN_LOCK_SZ - GLOBAL_SPIN_LOCK_1_ADDR) <= (1 * MB), "lock out of bounds, check mem.h");

// ============================================================================================================
// 6mb->7mb | size 1mb | Dram | used for flash controller's sq and sq maps
// ============================================================================================================
#define SQ_ENTRY_SZ_PER_CHANNEL (4 * KB)
#define SQ_BITMAP_SZ_PER_CHANNEL (32)

#define HW_CH_SQ_ENTRY_ADDR (MEM_BASE + 6 * MB)
#define HW_CH_SQ_BITMAP_ADDR (HW_CH_SQ_ENTRY_ADDR + TOTAL_CHANNEL * SQ_ENTRY_SZ_PER_CHANNEL)
static_assert((HW_CH_SQ_BITMAP_ADDR + TOTAL_CHANNEL * SQ_BITMAP_SZ_PER_CHANNEL - HW_CH_SQ_ENTRY_ADDR) <= (1 * MB), "sq out of bounds, check mem.h");

// ===========================================================================================================
// 7mb->8mb | size 1mb | Dram | used for flash controller's cq
// ===========================================================================================================
#define CQ_ENTRY_SZ_PER_CHANNEL (4 * KB)
#define HW_CH_CQ_ENTRY_ADDR (MEM_BASE + 7 * MB)
static_assert((TOTAL_CHANNEL * CQ_ENTRY_SZ_PER_CHANNEL) <= (1 * MB), "cq out of bounds, check mem.h");

// ===========================================================================================================
// 8mb->9mb | size 1mb | Dram | used for hcmd_entry.
// 256*(32~100Bytes)->  <  25KB, sz(bmp)=4*8=32Bytes.
// ===========================================================================================================
#define HCMD_BASE_ADDR (MEM_BASE + 8 * MB)

// ===========================================================================================================
// 9MB->10MB | size 1mb | Dram | used for task_queue? 1200Bytes * 6 -> < 10KBytes?
// ===========================================================================================================
#define FTL_TASK_Q (MEM_BASE + 9 * MB)

// ===========================================================================================================
// 10MB->11MB | size 1mb | Dram | used for nvme admin data buffer
// ===========================================================================================================
#define ADMIN_CMD_DRAM_DATA_BUFFER_ADDR (MEM_BASE + 10 * MB)

// ===========================================================================================================
// 100MB->110MB | size 10mb | Dram | define nvme io buffer 
// ===========================================================================================================
#define IO_DRAM_DATA_BUFFER_ADDR (MEM_BASE + 100 * MB)

// ===========================================================================================================
// 110MB->120MB | size 10mb | Dram | define buffer entry addr
// entry's size is 48 bytes, total num is buffer_cnt, totao size is 100k
// ===========================================================================================================
#define BUFFER_ENTRY_BASE_ADDR (MEM_BASE + 110 * MB) 

// ===========================================================================================================
// 120MB->130MB | size 10mb | Dram | define hcmd_buffer_entry
// ===========================================================================================================
#define HCMD_BUFFER_TABLE_BASE_ADDR (MEM_BASE + 120 * MB)

// ===========================================================================================================
// 130MB->140MB | size 10mb | define hash_buffer_entry
// ===========================================================================================================
#define HASH_TABLE_BASE_ADDR (MEM_BASE + 130 * MB)

// ===========================================================================================================
// 140mb->240mb | size 100mb | Dram | used for buffer
// ===========================================================================================================
#define BUFFER_BASE_ADDR (MEM_BASE + 140 * MB)
static_assert((BUFFER_NUMBER * (16 * KB)) <= (96 * MB), "buffer out of bounds, check mem.h");
// ===========================================================================================================
// 240mb->mb | size mb | Dram | used for glabal table
// ===========================================================================================================
#define GLOBAL_TABLE_ADDR (MEM_BASE + 240 * MB)
#define GLOBAL_TABLE_SZ (2 * FLASH_PAGE_SZ) 
// ===========================================================================================================
//  | size mb | Dram | used for l2p_table
// ===========================================================================================================
#define L2P_BASE_ADDR (MEM_BASE + 500 * MB)
#define L2P_TABLE_SIZE_LIMIT (300 * MB)
/* 
 * functions maninuplating MMU
 */
void init_core1_mmu();
void init_core0_mmu();

void validate_cache();
void disable_mmu();
void set_core1_mmu_table();
void enable_mmu();

#endif
