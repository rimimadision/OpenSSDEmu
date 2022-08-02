#ifndef _HCL_H
#define _HCL_H

#include "../config/config.h"
#include "../config/mem.h"
#include "../lib/type.h"
#include "../emu/fe/shmem.h"

#define HCMD_NUM (256)

extern u32 cache_invalide_tag_flag;

typedef enum _hcmd_opcode
{
    HCE_READ = 1,
    HCE_WRITE,
    HCE_ERASE,

} hcmd_opcode;

typedef enum _hcmd_l2p_status
{
    HCE_ISNOT_SEARCHED,
    HCE_IS_SEARCHED = 1,

} hcmd_l2p_status;

typedef enum _host_cmd_entry_status
{
    HCE_CHECK_CACHE, // core 0

    HCE_DATAMOVED, // core 0polling ,core0 and core1 add
    HCE_TO_SQ,     // core 0
    HCE_FROM_CQ,   // core 1
    HCE_FINISH,
    HCE_WAIT_CQ, // core0 add and core1 remove

    HCE_INIT // core 0
} host_cmd_entry_status;

typedef struct _host_cmd_entry
{
    unsigned int start_lsn;     // from nvme cmd
    unsigned int req_num;       // from nvme cmd ,请求粒度是4KB，请求个数是nvme_req_num
    unsigned int cur_cnt;       //记录分配的subpage个数，会和req_num 进行对比
    unsigned int cur_lsn;       // alloc buff
    unsigned int send_node_cnt; // send subbuffer cnt
    unsigned int cpl_cnt;    //记录返回的cq entry 的个数
    unsigned int cmdSlotTag; //通知dma时需要
	unsigned int nvme_dma_cpl;
    unsigned int opcode;
    unsigned int valid; // 0-free 1-valid
    host_cmd_entry_status status;
    hcmd_l2p_status is_searched;
#ifdef EMU
    u32 emu_id;
#endif
} host_cmd_entry;

u32 HCL_get_hcmd_entry_index(host_cmd_entry *hcmd_entry);
u32 HCL_init_host_cmd_entry_queue();
host_cmd_entry *HCL_get_hcmd_entry_addr(u32 hcmd_id);
host_cmd_entry *HCL_get_host_cmd_entry();

// u32 HCL_set_host_cmd_entry(host_cmd_entry **new_host_cmd_entry, u32 opcode, u32 startLA, u32 num, u32 cmdSlotTag);

u32 HCL_reclaim_hcmd_entry(host_cmd_entry *host_cmd_entry);
#endif
