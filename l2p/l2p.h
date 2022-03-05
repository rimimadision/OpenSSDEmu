#ifndef L2P_H
#define L2P_H

#include"../lib/type.h"
#include"../fcl/fcl.h"
#include"../config/config.h"
#include"../config/mem.h"

extern u32* lpn_ppn;
extern u8* ppn_state;

void L2P_init();
void L2P_calc_obj_ppa(u32 obj_id, u64 obj_offset ,phy_page_addr *ppa);
void L2P_calc_ppa(u32 lpn, phy_page_addr *ppa);
void L2P_ppn_items(u32 ppn_index, u32* ch, u32* block);
void L2P_search_ppn_gc_v1(u32 *ppn, u32 lpn, hcmd_opcode op_code) ;
void FTL_syn_erase_block(u32 ppn_index);

#endif
