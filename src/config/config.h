#ifndef CONFIG_H
#define CONFIG_H

#define OBJECT_MODE (1)
#define NORMAL_MODE (2)
#define FTL_MODE (NORMAL_MODE)

#define SLC (1)
#define TLC (2)
#define FLASH_TYPE (SLC)

#define NORMAL_CHANNEL \
    {                  \
        1, 1, 1, 1     \
    }
#define TOTAL_CHANNEL (4)

#define SINGLE_CORE_MODE (1)
#define MULTI_CORE_MODE (2)
#define CORE_MODE (MULTI_CORE_MODE)

#define YES (1)
#define NO (0)
#define BYPASS_BUFFER (NO)

#define PAGE_PER_BLOCK_SLC_MODE (384)

#define PAGE_PER_OBJECT (4 * 8 * 2 * 384)
#define BLOCK_PER_OBJECT (4 * 8 * 2)
#define BLOCK_PER_PLANE (1006)
#define USED 0
#define UNUSED 1

#define HW_QUEUE_ENTRY_NUMBER (256)
#define INVALID_VALUE (0xffffffff)
#define INVALID_ID (0xffffffff)

#define PAGE_SIZE (4 * 4096)
#define FLASH_PAGE_SZ PAGE_SIZE
#define SUB_PAGE_SIZE (4096)

#define PAGE_PER_BLOCK (384)

#define BLOCK_PER_CH (8 * 2 * 1006)
#define BLOCK_PER_CE (2 * 1006)
#define BLOCK_PER_LUN (2 * 1006)
#define BLOCK_PER_PLANE (1006)

#define PAGE_PER_CH (PAGE_PER_BLOCK * BLOCK_PER_CH)
#define PAGE_PER_CE (PAGE_PER_BLOCK * BLOCK_PER_CE)
#define PAGE_PER_LUN (PAGE_PER_BLOCK * BLOCK_PER_LUN)
#define PAGE_PER_PLANE (PAGE_PER_BLOCK * BLOCK_PER_PLANE)

#define PLANE_PER_LUN (2)
#define PLANE_PER_CE (2)
#define LUN_PER_CE (1)
#define CE_PER_CH (8)
#define CH_PER_BOARD (2)
#define PAGE_PER_CHANNEL (PAGE_PER_BLOCK * BLOCK_PER_CE * CE_PER_CH)

#define TOTAL_BOARD (1)
#define TOTAL_CH (TOTAL_BOARD * CH_PER_BOARD)
#define TOTAL_CE (TOTAL_CH * CE_PER_CH)
#define TOTAL_LUN (TOTAL_CE * LUN_PER_CE)
#define TOTAL_PLANE (TOTAL_LUN * PLANE_PER_LUN)
#define TOTAL_BLOCK (TOTAL_PLANE * BLOCK_PER_PLANE)
#define TOTAL_PAGE (TOTAL_BLOCK * PAGE_PER_BLOCK)

#include "../lib/type.h"
#define SSD_SZ (5u * (u64)GB)

#define F 0
#define V 1
#define I 2

// ROW means each plane select one block
#define PAGE_PER_ROW (PAGE_PER_BLOCK * PLANE_PER_LUN * LUN_PER_CE * CE_PER_CH * TOTAL_BOARD * CH_PER_BOARD)

// column means each plane select 3 page
#define PAGE_PER_COLUMN (3 * PLANE_PER_LUN)
// buffer define
#define BUFFER_NUMBER (5000)

// hardware define
#define HW_QUEUE_DEPTH (256)
#define INVALID_INDEX (0xffffffff)

/*
 * L2P related
 */

#endif
