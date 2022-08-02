#include <string.h>

#include "gc.h"
#include "../l2p/l2p.h"
#include "../hcl/hcl.h"
#include "../ftl/ftl_taskqueue.h"
#include "../config/config.h"
#include "../lib/type.h"

static inline void GC_set_pba(pba_entry *e, u32 ch, u32 ce, u32 lun, u32 plane, u32 block)
{
    assert(ch < TOTAL_CH);
    assert(ce < CE_PER_CH);
    assert(lun < LUN_PER_CE);
    assert(plane < PLANE_PER_LUN);
    assert(block < BLOCK_PER_PLANE);

    e->pba.ch = ch;
    e->pba.ce = ce;
    e->pba.lun = lun;
    e->pba.plane = plane;
    e->pba.block = block;
}

static inline boolean GC_inc_pba(pba_entry *e)
{
    if (e->pba.plane == PLANE_PER_LUN - 1)
    {
        e->pba.plane = 0;
        if (e->pba.ch == TOTAL_CH - 1)
        {
            e->pba.ch = 0;
            if (e->pba.ce == CE_PER_CH - 1)
            {
                e->pba.ce = 0;
                if (e->pba.lun == LUN_PER_CE - 1)
                {
                    e->pba.lun = 0;
                    if (e->pba.block == BLOCK_PER_PLANE - 1)
                    {
                        /* we can't increament pba anymore */
                        GC_set_pba(e, 0, 0, 0, 0, 0);
                        return FALSE;
                    }
                }
                else
                {
                    e->pba.lun++;
                }
            }
            else
            {
                e->pba.ce++;
            }
        }
        else
        {
            e->pba.ch++;
        }
    }
    else
    {
        e->pba.plane++;
    }

    return TRUE;
}

/* Initialize GC moudules and global variable of GC */
void GC_init_gc_moudle()
{
    GC_set_pba(&victim_block, 0, 0, 0, 0, 0);
}

/* Scan block_table to find victime blocks, and do GC to them */
void GC_main()
{
    if (TRUE == GC_search_victim_block())
    {
        GC_read_victim_block();
    }
}

/* find first block need to do GC (not most need), return TRUE means finded */
boolean GC_search_victim_block()
{
    pba_entry new_pba;
    /* find victim block from last victim_block */
    for (new_pba = victim_block;
         block_table_entry(new_pba.pba.ch,
                           new_pba.pba.ce,
                           new_pba.pba.lun,
                           new_pba.pba.plane,
                           new_pba.pba.block)
             .invalid_cnt < VICTIM_THRESH;)
    {
        if (FALSE == GC_inc_pba(&new_pba))
        {
            /* if can't find victim block, then return false */
            GC_set_pba(&victim_block, 0, 0, 0, 0, 0);
            return FALSE;
        }
    }

    /* We find pba available for GC */
    victim_block = new_pba;
}

/* read the valid page from victim block */
void GC_read_victim_block()
{
    /* get hcmd form HCL to send cmd for read victim block */
    host_cmd_entry hcmd = HCL_get_host_cmd_entry();

    /* fill the content of hcmd */

    /* send the hcmd to checkcache queue */
    FTL_sendhcmd(&hcmd, HCE_CHECK_CACHE);
}

/* write the valid page into victim block */
void GC_write_victim_block()
{
}