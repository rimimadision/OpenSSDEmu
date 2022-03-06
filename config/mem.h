#ifndef MEM_H
#define MEM_H

#include "../emu/emu_config.h"

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
0x00010000->0x0001ffff | 64kb->128kb | size 64kb | OCM | used for hcmd、task queue
---------------------------------------------------------------
0x00020000->0x0002ffff | 128kb->192kb | size 64kb | OCM | used for
---------------------------------------------------------------
0x00030000->0x000fffff | 192kb->1mb | size 832kb | Dram |
---------------------------------------------------------------
0x00100000->0x003fffff | 1mb->4mb | size 3mb | Dram | used for program、core0 mmutable
---------------------------------------------------------------
0x00400000->0x004fffff | 4mb->5mb | size 1mb | Dram | used for core1 mmutable. mmutable size is 4096*4=16kb
---------------------------------------------------------------
0x00500000->0x005fffff | 5mb->6mb | size 1mb | Dram | used for spin locks
---------------------------------------------------------------
0x00600000->0x006fffff | 6mb->7mb | size 1mb | Dram | used for flash controller's sq and sq maps. sizeof(sq)=256*4. sz(bmp)=4*8
---------------------------------------------------------------
0x00700000->0x007fffff | 7mb->8mb | size 1mb | Dram | used for flash controller's cq
---------------------------------------------------------------
0x00800000->0x008fffff | 8mb->9mb | size 1mb | Dram | used for hcmd_entry. 256*(32~100Bytes)->  <  25KB, sz(bmp)=4*8=32Bytes.
---------------------------------------------------------------
0x00900000->0x04000000 | 9mb->64mb | size 59mb | Dram | used for
---------------------------------------------------------------
0x04000000->0x3fffffff | 64mb->1024mb | size 960b | Dram | used for buffer

// 3GB Device memory
---------------------------------------------------------------
0x3fffffff->0xfff00000 | 4095mb->4096mb | size 1mb | D&O | used for
---------------------------------------------------------------
0xffff0000->0xffffffff | 4095mb->4096mb | size 256kb | D&O | used for core1 stack
*********************************************************************************/

// 0x00400000->0x004fffff | 4mb->5mb | size 1mb | Dram | used for core1 mmutable. mmutable size is 4096*4=16kb
#define CORE1_MMU_TABLE_BASE_ADDR (MEM_BASE + 4 * MB)

// 0x00500000->0x005fffff | 5mb->6mb | size 1mb | Dram | used for spin locks
#define GOBAL_SPIN_LOCK_1_ADDR (MEM_BASE + 5 * MB) //(770*1024*1024+5*4*1024)

#define CH0_SQ_SPIN_LOCK (GOBAL_SPIN_LOCK_1_ADDR + 0x04)
#define CH1_SQ_SPIN_LOCK (GOBAL_SPIN_LOCK_1_ADDR + 0x08)
#define CH2_SQ_SPIN_LOCK (GOBAL_SPIN_LOCK_1_ADDR + 0x0C)
#define CH3_SQ_SPIN_LOCK (GOBAL_SPIN_LOCK_1_ADDR + 0x10)

#define TQ_FROMCQ_SPIN_LOCK (CH3_SQ_SPIN_LOCK + 0x04)

#define TO_DATAMOVED_SPIN_LOCK (TQ_FROMCQ_SPIN_LOCK + 0x04)
#define TO_FINISH_SPIN_LOCK    (TO_DATAMOVED_SPIN_LOCK + 0x04)

#define LIST_ADD_SPIN_LOCK (TO_FINISH_SPIN_LOCK + 0x04)
#define LIST_DELETE_SPIN_LOCK (LIST_ADD_SPIN_LOCK + 0x04)
#define LIST_OP_SPIN_LOCK (LIST_DELETE_SPIN_LOCK + 0x04)

#define HCMD_MAP_SPIN_LOCK (LIST_OP_SPIN_LOCK + 0x04)

#define SYN_ERASE_SPIN_LOCK (HCMD_MAP_SPIN_LOCK + 0x04)

// 0x00600000->0x006fffff | 6mb->7mb | size 1mb | Dram | used for flash controller's sq and sq maps
#define HW_CH0_SQ_ENTRY_ADDR  (MEM_BASE + 6 * MB)
#define HW_CH1_SQ_ENTRY_ADDR  (HW_CH0_SQ_ENTRY_ADDR  + 4 * KB)
#define HW_CH2_SQ_ENTRY_ADDR  (HW_CH1_SQ_ENTRY_ADDR  + 4 * KB)
#define HW_CH3_SQ_ENTRY_ADDR  (HW_CH2_SQ_ENTRY_ADDR  + 4 * KB)
#define HW_CH0_SQ_BITMAP_ADDR (HW_CH3_SQ_ENTRY_ADDR  + 4 * KB)
#define HW_CH1_SQ_BITMAP_ADDR (HW_CH0_SQ_BITMAP_ADDR + 32)
#define HW_CH2_SQ_BITMAP_ADDR (HW_CH1_SQ_BITMAP_ADDR + 32)
#define HW_CH3_SQ_BITMAP_ADDR (HW_CH2_SQ_BITMAP_ADDR + 32)

// 0x00700000->0x007fffff | 7mb->8mb | size 1mb | Dram | used for flash controller's cq
#define HW_CH0_CQ_ENTRY_ADDR (MEM_BASE + 7 * MB)
#define HW_CH1_CQ_ENTRY_ADDR (HW_CH0_CQ_ENTRY_ADDR + 4 * KB)
#define HW_CH2_CQ_ENTRY_ADDR (HW_CH1_CQ_ENTRY_ADDR + 4 * KB)
#define HW_CH3_CQ_ENTRY_ADDR (HW_CH2_CQ_ENTRY_ADDR + 4 * KB)

// 0x00800000->0x008fffff | 8mb->9mb | size 1mb | Dram | used for hcmd_entry.
// 256*(32~100Bytes)->  <  25KB, sz(bmp)=4*8=32Bytes.
#define HCMD_BASE_ADDR (MEM_BASE + 8 * MB)

// 0x00900000->0x009fffff | 9mb->10mb | size 1mb | Dram | used for task_queue�� 1200Bytes * 6 -> < 10KBytes��
#define FTL_TASK_Q (MEM_BASE + 9 * MB)

// 0x00a00000->0x         | 10mb->320mb | size  | Dram | used for reserved

// 0x04000000->0x3fffffff | 320mb->1024mb | size 960b | Dram | used for buffer
// define buffer entry addr
#define BUFFER_ENTRY_BASE_ADDR (MEM_BASE + 500 * MB) //  entry's size is 48 bytes, total num is buffer_cnt, totao size is 100k

#define BUFFER_BASE_ADDR (MEM_BASE + 600 * MB)
// define hcmd_buffer_entry
#define HASH_TABLE_BASE_ADDR (MEM_BASE + 750 * MB)
// define hcmd_buffer_entry
#define HCMD_BUFFER_TABLE_BASE_ADDR (MEM_BASE + 800 * MB)

// 0x04000000->0x3fffffff | ->1024mb | size  | Dram | used for gc
#define GC_DATA_BUFFER_BASE_ADDR (MEM_BASE + 320 * MB)  // 7mb
#define GC_BLOCK_MAP_BASE_ADDR (MEM_BASE + 330 * MB)    // 2mb
#define GC_CH_MAP_BASE_ADDR (MEM_BASE + 340 * MB)       // 1mb
#define GC_VICTIM_BLOCK_BASE_ADDR (MEM_BASE + 350 * MB) // 1mb
#define GC_PPN_STATE (MEM_BASE + 360 * MB)              // 30mb

#define GC_COMMAND_LIST_ENTRY (MEM_BASE + 390 * MB) // 1mb
#define GC_LPN_PPN (MEM_BASE + 400 * MB)            //  100MB

// 2GB Device memory

//---------------------------------------------------------------
// 0x3fffffff->0xfff00000 | 4095mb->4096mb | size 1mb | D&O | used for device
//---------------------------------------------------------------
// 0xffff0000->0xffffffff | 4095mb->4096mb | size 1mb | D&O | used for core1 stack
//*********************************************************************************/
#ifndef EMU
#define FCTL_BASE_ADDR (0x60000000)
#else
#define FCTL_BASE_ADDR (FCL_BASE)
#endif

#define FCTL_BASE_SYS_REG (FCTL_BASE_ADDR + 0x00000000)
#define FCTL_BASE_SWQ_CMD_SRAM (FCTL_BASE_ADDR + 0x00100000)
#define FCTL_BASE_SWQ_SDR_SRAM (FCTL_BASE_ADDR + 0x00200000)
#define FCTL_BASE_SWQ_DDR_SRAM (FCTL_BASE_ADDR + 0x00300000)

#define FCTL_BASE_CH0_REG (FCTL_BASE_ADDR + 0x00800000)
#define FLASH_CTL_CH0_QUEUE_BASE_ADDR (FCTL_BASE_ADDR + 0x00900000)
#define FLASH_CTL_CH1_QUEUE_BASE_ADDR (FCTL_BASE_ADDR + 0x00B00000)
#define FLASH_CTL_CH2_QUEUE_BASE_ADDR (FCTL_BASE_ADDR + 0x00D00000)
#define FLASH_CTL_CH3_QUEUE_BASE_ADDR (FCTL_BASE_ADDR + 0x00F00000)

void init_core1_mmu();
void init_core0_mmu();

void validate_cache();
void disable_mmu();
void set_core1_mmu_table();
void enable_mmu();

#endif
