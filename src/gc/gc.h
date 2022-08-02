#ifndef _GC_H
#define _GC_H

#include <assert.h>

#include "../config/config.h"
#include "../lib/type.h"

static_assert(2 == sizeof(u16));

#define VICTIM_THRESH (PAGE_PER_BLOCK / 2)
/* 
 * struct for GC
 */
typedef union _pba_entry
{
    struct{
        u32 plane : 1;
        u32 block : 10;
        u32 lun : 1; 
        u32 ce : 3;
        u32 ch : 1;
    } pba;

    u16 addr;
}pba_entry;

/* 
 * global variable for GC
 */
pba_entry victim_block;

/* 
 * API for GC
 */
/* Scan block_table to find victime blocks, and do GC to them */
void GC_main();

/* Initialize GC moudules and global variable of GC */
void GC_init_gc_moudle();

/* find first block need to do GC (not most need), return TRUE means finded */
boolean GC_search_victim_block();

/* read the valid page from victim block */
void GC_read_victim_block();

/* write the valid page into victim block */
void GC_write_victim_block();

#endif