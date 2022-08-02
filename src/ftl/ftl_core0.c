#include <string.h>

#include "ftl_core0.h"
#include "ftl_taskqueue.h"

#include "../config/config.h"
#include "../lib/dprint.h"
#include "../lib/lock.h"

#include "../hcl/hcl.h"
#include "../l2p/l2p.h"
#include "../buffer/buffer.h"
#include "../nvme/host_lld.h"
#include "../emu/be/be.h"

/**********************************************************************************
Func    Name: FTL_handle_checkcache
Descriptions: handle check cache status 's task queue entry
Input   para: host_cmd_entry
In&Out  Para:
Output  para:
Return value: SUCCESS OR FAIL
***********************************************************************************/
u32 FTL_handle_checkcache(host_cmd_entry *hcmd_entry)
{
    if (hcmd_entry == NULL)
    {
        debug_printf("[ASSERT]: [FTL_handle_checkcache]hcmd_entry=NULL \n");
        return FAIL;
    }

    u32 opcode = hcmd_entry->opcode;
    u32 cur_cnt = hcmd_entry->cur_cnt;
    u32 req_num = hcmd_entry->req_num;
    u32 cur_lsn = hcmd_entry->cur_lsn;
    l2p_table_entry l2p_entry;
    u32 hcmd_id = HCL_get_hcmd_entry_index(hcmd_entry);
    u32 buf_id;
    u32 ret;
    buffer_using_state using_state;
    while (cur_cnt < req_num)
    {

        l2p_entry = L2P_get_l2p_entry(cur_lsn, opcode, hcmd_entry->is_searched);

        ret = BUFF_check_buf_hit(l2p_entry.psa, opcode, &buf_id);

        // if there is a cache hit.
        if (ret == BUF_HIT)
        {
            BUFF_hcmd_add_buffer(hcmd_id, buf_id, l2p_entry.psa.slc_psa.subpage);
        }
        else if (ret == BUF_NOT_HIT)
        {
            buf_id = BUFF_allocate_buf(l2p_entry.psa, opcode);
            if (buf_id != INVALID_ID)
            {
                BUFF_hcmd_add_buffer(hcmd_id, buf_id, l2p_entry.psa.slc_psa.subpage);
            }
            else
            {
                hcmd_entry->cur_cnt = cur_cnt;
                // ISSUE: What for buf_cnt
                // hcmd_entry->buf_cnt = cur_cnt;
                hcmd_entry->cur_lsn = cur_lsn;

                //  if ((opcode == HCE_WRITE) && (hcmd_entry->is_searched == HCE_ISNOT_SEARCHED))
                if (hcmd_entry->is_searched == HCE_ISNOT_SEARCHED)
                {
                    cur_cnt++;
                    cur_lsn++;
                    while (cur_cnt < req_num)
                    {

                        l2p_entry = L2P_get_l2p_entry(cur_lsn, opcode, hcmd_entry->is_searched);
                        cur_cnt++;
                        cur_lsn++;
                    }
                }
                hcmd_entry->is_searched = HCE_IS_SEARCHED;
                FTL_sendhcmd(hcmd_entry, HCE_CHECK_CACHE);
                return FAIL;
            }
        }
        else
        {
            hcmd_entry->cur_cnt = cur_cnt;
            // hcmd_entry->buf_cnt = cur_cnt;

            hcmd_entry->cur_lsn = cur_lsn;
            //  if ((opcode == HCE_WRITE) && (hcmd_entry->is_searched == HCE_ISNOT_SEARCHED))
            if (hcmd_entry->is_searched == HCE_ISNOT_SEARCHED)
            {
                cur_cnt++;
                cur_lsn++;
                while (cur_cnt < req_num)
                {

                    l2p_entry = L2P_get_l2p_entry(cur_lsn, opcode, hcmd_entry->is_searched);
                    cur_cnt++;
                    cur_lsn++;
                }
            }
            hcmd_entry->is_searched = HCE_IS_SEARCHED;
            FTL_sendhcmd(hcmd_entry, HCE_CHECK_CACHE);
            return FAIL;
        }
        cur_cnt++;
        cur_lsn++;
    }

    hcmd_entry->cur_cnt = cur_cnt;
    hcmd_entry->cur_lsn = cur_lsn;
    // hcmd_entry->buf_cnt = cur_cnt;
    if (opcode == HCE_WRITE)
    {
        FTL_sendhcmd(hcmd_entry, HCE_DATAMOVED);
    }
    else if (opcode == HCE_READ)
    {
        FTL_sendhcmd(hcmd_entry, HCE_TO_SQ);
    }

    return SUCCESS;
}

/**********************************************************************************
Func    Name: FTL_handle_tosq
Descriptions: handle to sq status 's task queue entry
Input   para: host_cmd_entry
In&Out  Para:
Output  para:
Return value: SUCCESS OR FAIL
***********************************************************************************/
u32 FTL_handle_tosq(host_cmd_entry *hcmd_entry)
{
    if (hcmd_entry == NULL)
    {
        debug_printf("[ASSERT]: [FTL_handle_tosq]hcmd_entry=NULL \n");
        return FAIL;
    }
    u32 op_code = hcmd_entry->opcode;
    u32 hcmd_id = HCL_get_hcmd_entry_index(hcmd_entry);
    //hcmd_entry->buf_cnt = BUFF_get_hcmd_buff_cnt(hcmd_id);
    u32 send_node_cnt = hcmd_entry->send_node_cnt;
    u32 cur_cnt = hcmd_entry->cur_cnt;
    u32 sq_index = 0;
    psa_entry psa;
    phy_page_addr ppa;
    u32 buf_index;
    u32 ret = SUCCESS;
    u32 subpage;
    while (send_node_cnt < cur_cnt)
    {

        buf_index = BUFF_hcmd_get_buffer(hcmd_id, send_node_cnt, &subpage);

        BUFF_node_add_buffer(subpage, buf_index, op_code);
        BUFF_get_buff_psa(buf_index, &psa);

        if (op_code == HCE_WRITE)
        {
            //if (BUFF_get_node_cnt(psa, HCE_WRITE) >= (PAGE_SIZE / SUB_PAGE_SIZE))
            //{
            if (BUFF_in_one_page_cnt(psa, HCE_WRITE) == (PAGE_SIZE / SUB_PAGE_SIZE))
            {
                sq_index = FCL_get_free_SQ_entry(psa.slc_psa.ch);
                if (sq_index == INVALID_ID)
                {
                    FTL_sendhcmd(hcmd_entry, HCE_TO_SQ);
                    hcmd_entry->send_node_cnt = send_node_cnt;
                    EMU_log_println(ERR, "ch %d,emid %d", psa.slc_psa.ch, HCL_get_hcmd_entry_addr(hcmd_id)->emu_id);

                    // assert(0);
                    return FAIL;
                }
                else
                {
                    buffer_entry be = buffer_list_ptr->buffer_entry[buf_index];
                    memset(&ppa, 0, sizeof(ppa));
                    ppa.nand_flash_addr.slc_mode.block = be.psa.slc_psa.block;
                    ppa.nand_flash_addr.slc_mode.page = be.psa.slc_psa.page;
                    ppa.nand_flash_addr.slc_mode.lun = be.psa.slc_psa.lun;
                    ppa.nand_flash_addr.slc_mode.plane = be.psa.slc_psa.plane;
                    ppa.ch = be.psa.slc_psa.ch;
                    ppa.ce = be.psa.slc_psa.ce;
                    FCL_set_SQ_entry(hcmd_id, sq_index, buf_index, op_code, &ppa, HCMD_SPACE);

                    do
                    {
                        ret = FCL_send_SQ_entry(sq_index, be.psa.slc_psa.ch, be.psa.slc_psa.ce);
                    } while (ret == FAIL);

                    BUFF_dec_node_cnt(psa, HCE_WRITE, (PAGE_SIZE / SUB_PAGE_SIZE));
                    BUFF_node_de_subbuffer(&(buffer_list_ptr->buffer_entry[buf_index].sub_buf[0].fcl_buf_list_node), (PAGE_SIZE / SUB_PAGE_SIZE));
                }
            }
            //}
        }
        send_node_cnt++;
    }
    hcmd_entry->send_node_cnt = send_node_cnt;
    return SUCCESS;
}

/**********************************************************************************
Func    Name: FTL_handle_datamove
Descriptions: handle data move status 's task queue entry
Input   para: host_cmd_entry
In&Out  Para:
Output  para:
Return value: SUCCESS OR FAIL
***********************************************************************************/
u32 FTL_handle_datamove(host_cmd_entry *hcmd_entry)
{
    if (hcmd_entry == NULL)
    {
        debug_printf("[FTL_handle_checkcache]hcmd_entry=NULL \n");
        return FAIL;
    }

    u32 op_code = hcmd_entry->opcode;
    u32 hcmd_id = HCL_get_hcmd_entry_index(hcmd_entry);
    //  hcmd_entry->buf_cnt = BUFF_get_hcmd_buff_cnt(hcmd_id);
    u32 buf_cnt = hcmd_entry->cur_cnt;
    u32 cmd_slot_tag = hcmd_entry->cmdSlotTag;
    u32 subpage;
    u32 buf_id;
    u32 buf_4KB_addr;
    u32 ret;
    if (op_code == HCE_READ)
    {
        while (hcmd_entry->nvme_dma_cpl < buf_cnt)
        {
            buf_id = BUFF_hcmd_get_buffer(hcmd_id, hcmd_entry->nvme_dma_cpl, &subpage);
            buf_4KB_addr = BUFF_get_buffer_addr(buf_id);

            set_auto_tx_dma(cmd_slot_tag, hcmd_entry->nvme_dma_cpl, buf_4KB_addr + SUB_PAGE_SIZE * subpage);
            BUFF_set_subpage_dirty(buf_id, subpage);
            hcmd_entry->nvme_dma_cpl++;
        }

        FTL_sendhcmd(hcmd_entry, HCE_FINISH);
    }
    else
    {
        while (hcmd_entry->nvme_dma_cpl < buf_cnt)
        {
            buf_id = BUFF_hcmd_get_buffer(hcmd_id, hcmd_entry->nvme_dma_cpl, &subpage);
            buf_4KB_addr = BUFF_get_buffer_addr(buf_id);

            set_auto_rx_dma(cmd_slot_tag, hcmd_entry->nvme_dma_cpl, buf_4KB_addr + SUB_PAGE_SIZE * subpage);
            // there is 5.12us for each 4KB dma translation.
            //#include"sleep.h"
            //                usleep(8);
            BUFF_set_subpage_dirty(buf_id, subpage);
            hcmd_entry->nvme_dma_cpl++;
        }

        ret = FTL_sendhcmd(hcmd_entry, HCE_TO_SQ); //HCE_TO_SQ  HCE_FINISH
        if (ret == FAIL)
        {
            xil_printf("handle_datamove has error !\n");
        }
    }

    return SUCCESS;
}

/**********************************************************************************
Func    Name: FTL_handle_fromcq
Descriptions: handle from cq status 's task queue entry
Input   para: host_cmd_entry
In&Out  Para:
Output  para:
Return value: SUCCESS OR FAIL
***********************************************************************************/
u32 FTL_handle_fromcq(host_cmd_entry *hcmd_entry)
{
    if (hcmd_entry == NULL)
    {
        //debug_printf("[INFO] [FTL_handle_fromcq]hcmd_entry=NULL \n");
        return FAIL;
    }

    if (hcmd_entry->opcode == HCE_WRITE)
    {
        hcmd_entry->status = HCE_FINISH;
        FTL_sendhcmd(hcmd_entry, HCE_FINISH);
    }
    else
    {
        hcmd_entry->status = HCE_DATAMOVED;
        FTL_sendhcmd(hcmd_entry, HCE_DATAMOVED);
    }

    return SUCCESS;
}

/**********************************************************************************
Func    Name: FTL_handle_fromcq
Descriptions: handle finish status 's task queue entry
Input   para: host_cmd_entry
In&Out  Para:
Output  para:
Return value: SUCCESS OR FAIL
***********************************************************************************/
u32 FTL_handle_finish(host_cmd_entry *hcmd_entry)
{
    if (hcmd_entry == NULL)
    {
        //debug_printf("[INFO] [FTL_handle_finish]hcmd_entry=NULL \n");
        return FAIL;
    }

    BUFF_hcmd_free_buf(HCL_get_hcmd_entry_index(hcmd_entry));
    HCL_reclaim_hcmd_entry(hcmd_entry);

    return SUCCESS;
}

void FTL_polling_read_list(void)
{
    u32 ch, ce, lun, plane;
    u32 sq_index;
    psa_entry psa;
    phy_page_addr ppa;
    u32 buf_index;
    u32 ret = SUCCESS;
    u32 hcmd_id;
    u32 cnt = 0;
    buffer_entry *buf = NULL;
    sub_buffer_entry *subbuf = NULL;
    for (lun = 0; lun < LUN_PER_CE; lun++)
    {
        for (ce = 0; ce < CE_PER_CH; ce++)
        {
            for (ch = 0; ch < TOTAL_CH; ch++)
            {
                for (plane = 0; plane < PLANE_PER_LUN; plane++)
                {
                    psa.addr = 0;
                    psa.slc_psa.ch = ch;
                    psa.slc_psa.ce = ce;
                    psa.slc_psa.lun = lun;
                    psa.slc_psa.plane = plane;
                    if (BUFF_get_node_cnt(psa, HCE_READ) > 0)
                    {
                        subbuf = container_of((&node_table.node_read_list[ch][ce][lun][plane])->next, typeof(*subbuf), fcl_buf_list_node);

                        if (subbuf == NULL)
                        {
                            assert(0);
                        }
                        buf = &buffer_list_ptr->buffer_entry[subbuf->buf_id];
                        psa = buf->psa;
                        if (subbuf->buf_id >= BUFFER_NUMBER)
                        {
                            assert(0);
                        }
                        if ((ch != psa.slc_psa.ch) || (ce != psa.slc_psa.ce) || (lun != psa.slc_psa.lun) || ((plane != psa.slc_psa.plane)))
                        {
                            assert(0);
                        }

                        sq_index = FCL_get_free_SQ_entry(ch);
                        if (sq_index == INVALID_ID)
                        {

                            continue;
                        }
                        else
                        {
                            memset(&ppa, 0, sizeof(ppa));
                            ppa.nand_flash_addr.slc_mode.block = psa.slc_psa.block;
                            ppa.nand_flash_addr.slc_mode.page = psa.slc_psa.page;
                            ppa.nand_flash_addr.slc_mode.lun = psa.slc_psa.lun;
                            ppa.nand_flash_addr.slc_mode.plane = psa.slc_psa.plane;
                            ppa.ch = psa.slc_psa.ch;
                            ppa.ce = psa.slc_psa.ce;
                            hcmd_id = subbuf->hcmd_id;
                            cnt = BUFF_in_one_page_cnt(psa, HCE_READ);
                            FCL_set_SQ_entry(hcmd_id, sq_index, buf->buf_id, HCE_READ, &ppa, HCMD_SPACE);
                            //??
                            do
                            {
                                ret = FCL_send_SQ_entry(sq_index, ch, ce);
                            } while (ret == FAIL);

                            BUFF_dec_node_cnt(psa, HCE_READ, cnt);
                            BUFF_node_de_subbuffer(&(subbuf->fcl_buf_list_node), cnt);
                        }
                    }
                }
            }
        }
    }
}
void FTL_polling_write_list(void)
{
    u32 ch, ce, lun, plane;
    u32 sq_index;
    psa_entry psa;
    phy_page_addr ppa;
    u32 buf_index;
    u32 ret = SUCCESS;
    u32 hcmd_id;
    u32 cnt = 0;
    u32 pre_read_bufid;
    u32 subpage;
    u32 addr, i;
    buffer_entry *buf = NULL;
    sub_buffer_entry *subbuf = NULL;
    for (lun = 0; lun < LUN_PER_CE; lun++)
    {
        for (ce = 0; ce < CE_PER_CH; ce++)
        {
            for (ch = 0; ch < TOTAL_CH; ch++)
            {
                for (plane = 0; plane < PLANE_PER_LUN; plane++)
                {
                    psa.addr = 0;
                    psa.slc_psa.ch = ch;
                    psa.slc_psa.ce = ce;
                    psa.slc_psa.lun = lun;
                    psa.slc_psa.plane = plane;
                    if (node_table.node_write_list[ch][ce][lun][plane].next != &node_table.node_write_list[ch][ce][lun][plane])
                    {
                        subbuf = container_of((&node_table.node_write_list[ch][ce][lun][plane])->next, typeof(*subbuf), fcl_buf_list_node);

                        // list_for_each_entry(subbuf, &(node_table.node_write_list[ch][ce][lun][plane]), fcl_buf_list_node)
                        {
                            if (subbuf == NULL)
                            {
                                assert(0);
                            }
                            if (subbuf->buf_id >= BUFFER_NUMBER)
                            {
                                assert(0);
                            }
                            buf = &buffer_list_ptr->buffer_entry[subbuf->buf_id];
                            psa = buf->psa;
                            if ((ch != psa.slc_psa.ch) || (ce != psa.slc_psa.ce) || (lun != psa.slc_psa.lun) || ((plane != psa.slc_psa.plane)))
                            {
                                assert(0);
                            }
                            if (buf->opcode == HCE_READ)
                            {
                                assert(0);
                            }
                            sq_index = FCL_get_free_SQ_entry(ch);
                            if (sq_index == INVALID_ID)
                            {

                                continue; //?
                            }
                            else
                            {
                                memset(&ppa, 0, sizeof(ppa));
                                ppa.nand_flash_addr.slc_mode.block = psa.slc_psa.block;
                                ppa.nand_flash_addr.slc_mode.page = psa.slc_psa.page;
                                ppa.nand_flash_addr.slc_mode.lun = psa.slc_psa.lun;
                                ppa.nand_flash_addr.slc_mode.plane = psa.slc_psa.plane;
                                ppa.ch = psa.slc_psa.ch;
                                ppa.ce = psa.slc_psa.ce;
                                hcmd_id = subbuf->hcmd_id;
                                cnt = BUFF_in_one_page_cnt(psa, HCE_WRITE);
                                if ((cnt == PAGE_SIZE / SUB_PAGE_SIZE) || (buf->opcode == BUF_PREREAD_DONE))
                                {
                                    FCL_set_SQ_entry(hcmd_id, sq_index, buf->buf_id, HCE_WRITE, &ppa, HCMD_SPACE);

                                    do
                                    {
                                        ret = FCL_send_SQ_entry(sq_index, ch, ce);
                                    } while (ret == FAIL);

                                    BUFF_dec_node_cnt(psa, HCE_WRITE, cnt);
                                    BUFF_node_de_subbuffer(&(subbuf->fcl_buf_list_node), cnt);
                                    if (buf->opcode == BUF_PREREAD_DONE)
                                    {
                                        buf->opcode = BUF_WRITE;
                                        for (i = 0; i < 4; i++)
                                        {
                                            if (buf->sub_buf[i].using_state == WAIT_PRE_READ)
                                            {
                                                if (buf->sub_buf[i].dirty == 0)

                                                {
                                                    //      assert(0);
                                                }
                                            }
                                        }
                                    }
                                }
                                else if (BUFF_in_one_page_all_dirty(buf->buf_id) == TRUE)
                                {
                                    FCL_set_SQ_entry(hcmd_id, sq_index, buf->buf_id, HCE_WRITE, &ppa, HCMD_SPACE);

                                    do
                                    {
                                        ret = FCL_send_SQ_entry(sq_index, ch, ce);
                                    } while (ret == FAIL);

                                    BUFF_dec_node_cnt(psa, HCE_WRITE, cnt);
                                    BUFF_node_de_subbuffer(&(subbuf->fcl_buf_list_node), cnt);
                                }
                                else
                                {
                                    //PRE READ BEFORE WRITE  maybe improve
                                    if (BUFF_get_buff_using_state(buf->buf_id, subbuf->subpage) == USING)
                                    {
                                        psa.addr = psa.addr | subbuf->subpage;
                                        psa = BUFF_cal_preread_psa(psa, cnt);
                                        ret = BUFF_check_buf_hit(psa, BUF_PREREAD, &pre_read_bufid);

                                        // if there is a cache hit.
                                        if (ret == BUF_NOT_HIT)
                                        {
                                            pre_read_bufid = BUFF_allocate_buf(psa, BUF_PREREAD);
                                        }
                                        if (ret == BUF_OCCUPIED)
                                        {
                                            FCL_free_SQ_entry(ch, sq_index);
                                            continue;
                                        }
                                        if (pre_read_bufid == INVALID_ID)
                                        {
                                            FCL_free_SQ_entry(ch, sq_index);
                                            continue;
                                        }
                                        BUFF_set_subpage_hcmdid(pre_read_bufid, psa.slc_psa.subpage, hcmd_id);
                                        FCL_set_SQ_entry(hcmd_id, sq_index, pre_read_bufid, HCE_READ, &ppa, HCMD_SPACE);

                                        do
                                        {
                                            ret = FCL_send_SQ_entry(sq_index, ch, ce);
                                        } while (ret == FAIL);
                                        for (i = 0; i < PAGE_SIZE / SUB_PAGE_SIZE; i++)
                                        {
                                            if ((BUFF_get_buff_using_state(buf->buf_id, i) == USING) && (buf->sub_buf[i].fcl_buf_list_node.next != NULL))
                                            {
                                                BUFF_set_buff_using_state(buf->buf_id, WAIT_PRE_READ, i);
                                            }
                                        }
                                        for (i = 0; i < 4; i++)
                                        {
                                            if (buf->sub_buf[i].using_state == WAIT_PRE_READ)
                                            {
                                                if (buf->sub_buf[i].dirty == 0)

                                                {
                                                    assert(0);
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        FCL_free_SQ_entry(ch, sq_index);
                                    }
                                    // continue; //??
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
