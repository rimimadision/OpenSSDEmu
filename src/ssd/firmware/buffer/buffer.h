#ifndef BUFFER_H
#define BUFFER_H

#include "../lib/type.h"
#include "../lib/list.h"
#include "../config/config.h"
#include "../l2p/l2p.h"

#define NODE_NUM (TOTAL_LUN)
#define HASH_TABLE_SIZE (1000) //??
#define INVALID_ID (0xffffffff)
#define BUF_HIT (0)
#define BUF_NOT_HIT (1)
#define BUF_OCCUPIED (2)
#ifndef HCMD_NUM
#define HCMD_NUM (256)
#endif
#define BUFF_SIZE (4096 * 4)
typedef enum _buffer_is_using
{
	USING,
	NOT_USING,
	WAIT_PRE_READ,
} buffer_using_state;
typedef enum _buffer_hit_state
{
	BE_HIT,  //read buffer hit
	BE_UNHIT //read buffer no hit
} buffer_hit_state;
typedef enum _buffer_dirty_state
{
	BUF_CLEAN, //read buffer hit
	BUF_DIRTY  //read buffer no hit
} buffer_dirty_state;

typedef enum _buff_opcode
{
	BUF_FREE,
	BUF_READ = 1,
	BUF_WRITE,
	BUF_PREREAD,
	BUF_PREREAD_DONE,
	BUF_PREREAD_RECLAIM,
} buff_opcode;
typedef struct _hash_table
{
	list_head hash_table[HASH_TABLE_SIZE];
} hash_table;

typedef struct _hcmd_buf_table
{
	list_head table_entry[HCMD_NUM];
	u32 cnt[HCMD_NUM];

} hcmd_buf_table;
typedef struct _sub_buffer_entry
{
	u32 buf_id;
	u32 hcmd_id;
	u16 dirty;
	u16 subpage;
	buffer_using_state using_state;
	list_node hcmd_buf_list_node;
	list_node fcl_buf_list_node;
} sub_buffer_entry;

typedef struct _buffer_entry
{
	psa_entry psa;
	u32 buf_id;
	buff_opcode opcode;
	sub_buffer_entry sub_buf[PAGE_SIZE / SUB_PAGE_SZ];
	list_node free_list_node;
	list_node lru_list_node;
	list_node hash_table_node;
} buffer_entry;

typedef struct _buffer_list
{
	buffer_entry buffer_entry[BUFFER_NUMBER];
} buffer_list;

buffer_list *buffer_list_ptr;

#define INVALID_BUFF_ID (0xffffffff)
#define INVALID_BUFF_INDEX (0xffffffff)

typedef struct _fcl_node_list
{
	list_head node_read_list[TOTAL_CH][CE_PER_CH][LUN_PER_CE][PLANE_PER_LUN];
	list_head node_write_list[TOTAL_CH][CE_PER_CH][LUN_PER_CE][PLANE_PER_LUN];
	u32 node_write_cnt[TOTAL_CH][CE_PER_CH][LUN_PER_CE][PLANE_PER_LUN];
	u32 node_read_cnt[TOTAL_CH][CE_PER_CH][LUN_PER_CE][PLANE_PER_LUN];
} fcl_node_list;

fcl_node_list node_table;

u32 BUFF_get_hcmd_buff_cnt(u32 hcmd_id);
u32 BUFF_get_hcmd_id(u32 buf_id, u32 subpage);
u32 BUFF_hcmd_get_buffer(u32 hcmd_index, u32 offset, u32 *subpage); //ok
u32 BUFF_get_buffer_addr(u32 buff_id);
u32 BUFF_get_buffer_id(u32 buff_addr);
u32 BUFF_get_hcmd_buff_cnt(u32 hcmd_index);						//ok
u32 BUFF_allocate_buf(psa_entry psa, u32 opcode);				//ok
u32 BUFF_check_buf_hit(psa_entry psa, u32 opcode, u32 *buf_id); //ok
void BUFF_hcmd_free_buf(u32 hcmd_index);						//ok

void BUFF_hcmd_add_buffer(u32 hcmd_id, u32 buffer_id, u32 subpage); //ok
void BUFF_node_add_buffer(u32 subpage, u32 buffer_id, buff_opcode op);
void BUFF_init_buffer(); //ok
void BUFF_dump_lru_list();
void BUFF_set_buff_psa(u32 buf_id, psa_entry psa);
void BUFF_set_subpage_hcmdid(u32 buf_id, u32 subpage, u32 hcmd_id);

u32 BUFF_get_node_cnt(psa_entry psa, buff_opcode op);
void BUFF_dec_node_cnt(psa_entry psa, buff_opcode op, u32 cnt);
//void BUFF_node_de_subbuffer(psa_entry psa, buff_opcode op, u32 cnt); // ok
void BUFF_node_de_subbuffer(list_node *node, u32 cnt);
void BUFF_get_buff_psa(u32 buffer_id, psa_entry *psa);   // ok
u32 BUFF_in_one_page_cnt(psa_entry psa, buff_opcode op); // ok

void BUF_preread_done(u32 buf_id);
psa_entry BUFF_cal_preread_psa(psa_entry psa, u32 cnt);									 // Ok
void BUFF_set_buff_using_state(u32 buf_id, buffer_using_state using_state, u32 subpage); //ok
buffer_using_state BUFF_get_buff_using_state(u32 buf_id, u32 subpage);					 //ok
boolean BUFF_in_one_page_all_dirty(u32 buf_id);
void BUFF_set_subpage_dirty(u32 buf_id, u32 subpage);

void BUFF_unit_test();
void BUFF_unit_test1();
void BUFF_unit_test2();
void BUFF_unit_test3();
#endif
