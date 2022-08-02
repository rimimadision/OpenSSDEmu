#include "buffer.h"

#include "../config/config.h"
#include "../config/mem.h"
#include "../lib/dprint.h"
#include <string.h>
#include <stdlib.h>

hash_table *hash_table_ptr;
hcmd_buf_table *hcmd_buf_table_ptr;

boolean buf_is_free(u32 buf_id);

fcl_node_list node_table;
list_head free_buf_list_head = {&free_buf_list_head, &free_buf_list_head};
list_head lru_buf_list_head = {&lru_buf_list_head, &lru_buf_list_head};
/* 
 * static internal function 
 */
static inline void init_hash_table()
{
	hash_table_ptr = (hash_table *)(HASH_TABLE_BASE_ADDR);

	/* Set all hash entry link 0 buffer */
	for (u32 i = 0; i < HASH_TABLE_SIZE; i++)
	{
		hash_table_ptr->hash_table[i].next = &hash_table_ptr->hash_table[i];
		hash_table_ptr->hash_table[i].prev = &hash_table_ptr->hash_table[i];
	}
}

static inline u32 get_hash_code(u32 key)
{
	return key % HASH_TABLE_SIZE;
}

static inline buffer_entry *search_hash_table_entry(u32 key)
{
	buffer_entry *buf;

	u32 hash_value = get_hash_code(key);
	list_for_each_entry(buf, &hash_table_ptr->hash_table[hash_value], hash_table_node)
	{
		if (buf->psa.addr == key)
		{
			//			EMU_log_println(LOG, "find the buffer %u", buf->buf_id);
			return buf;
		}
	}

	return NULL;
}

static inline void insert_hash_table_entry(u32 key, buffer_entry *be)
{
	u32 hash_value = get_hash_code(key);
	//	EMU_log_println(LOG, "insert with hash val %u buf id is %u", hash_value, be->buf_id);
	list_add_tail(&be->hash_table_node, &hash_table_ptr->hash_table[hash_value]);
}

static inline void delete_hash_table_entry(u32 key, buffer_entry *buf)
{
	buffer_entry *ser_buf;
	u32 hash_value = get_hash_code(key);
#if 1
	list_for_each_entry(ser_buf, &hash_table_ptr->hash_table[hash_value], hash_table_node)
	{
		//		EMU_log_println(LOG, "hash %u", hash_value);
		if (ser_buf->psa.addr == key)
		{
			if (ser_buf != buf)
			{
				assert(0);
			}
			list_delete(&ser_buf->hash_table_node);
			//xil_printf("list_delete(&buf->hash_table_node) %d \n", buf->buf_id);
			return;
		}
	}
#endif
	list_delete(&buf->hash_table_node);
	assert(0);
}

static inline void init_hcmd_buf_table()
{
	hcmd_buf_table_ptr = (hcmd_buf_table *)(HCMD_BUFFER_TABLE_BASE_ADDR);

	for (u32 i = 0; i < HCMD_NUM; i++)
	{
		hcmd_buf_table_ptr->table_entry[i].next = &hcmd_buf_table_ptr->table_entry[i];
		hcmd_buf_table_ptr->table_entry[i].prev = &hcmd_buf_table_ptr->table_entry[i];
		hcmd_buf_table_ptr->cnt[i] = 0;
	}
}

static inline void init_fcl_node_table()
{
	u32 ch, ce, lun, plane;

	for (ch = 0; ch < TOTAL_CH; ch++)
	{
		for (ce = 0; ce < CE_PER_CH; ce++)
		{
			for (lun = 0; lun < LUN_PER_CE; lun++)
			{
				for (plane = 0; plane < PLANE_PER_LUN; plane++)
				{
					node_table.node_read_list[ch][ce][lun][plane].next = &node_table.node_read_list[ch][ce][lun][plane];
					node_table.node_read_list[ch][ce][lun][plane].prev = &node_table.node_read_list[ch][ce][lun][plane];

					node_table.node_write_list[ch][ce][lun][plane].next = &node_table.node_write_list[ch][ce][lun][plane];
					node_table.node_write_list[ch][ce][lun][plane].prev = &node_table.node_write_list[ch][ce][lun][plane];

					node_table.node_write_cnt[ch][ce][lun][plane] = 0;
					node_table.node_read_cnt[ch][ce][lun][plane] = 0;
				}
			}
		}
	}
}

static inline void dump_free_buf_list()
{
	buffer_entry *buf;

	list_for_each_entry(buf, &free_buf_list_head, free_list_node)
	{
		xil_printf("dump_free_buf_list, buf_id is %d \n", buf->buf_id);
	}
}

static inline void dump_lru_buf_list()
{
	buffer_entry *buf;

	list_for_each_entry(buf, &lru_buf_list_head, lru_list_node)
	{
		xil_printf("dump_lru_buf_list, buf_id is %d \n", buf->buf_id);
	}
}

/**********************************************************************************
Func    Name: BUFF_init_buffer
Descriptions: None
Input   para: None
In&Out  Para: None
Output  para: None
Return value: None
***********************************************************************************/
void BUFF_init_buffer()
{
#if (BYPASS_BUFFER == YES)

#else
	u32 i, j;
	// init hash table
	init_hash_table();

	// init hcmd buf table
	init_hcmd_buf_table();

	init_fcl_node_table();
	// init buffer space
	buffer_list_ptr = (buffer_list *)(BUFFER_ENTRY_BASE_ADDR);
	for (i = 0; i < BUFFER_NUMBER; i++)
	{
		buffer_list_ptr->buffer_entry[i].psa.addr = 0;
		buffer_list_ptr->buffer_entry[i].buf_id = i;
		buffer_list_ptr->buffer_entry[i].opcode = BUF_FREE;
		buffer_list_ptr->buffer_entry[i].free_list_node.next = NULL;
		buffer_list_ptr->buffer_entry[i].free_list_node.prev = NULL;
		buffer_list_ptr->buffer_entry[i].lru_list_node.next = NULL;
		buffer_list_ptr->buffer_entry[i].lru_list_node.prev = NULL;
		buffer_list_ptr->buffer_entry[i].hash_table_node.next = NULL;
		buffer_list_ptr->buffer_entry[i].hash_table_node.prev = NULL;

		for (j = 0; j < PAGE_SIZE / SUB_PAGE_SZ; j++)
		{
			buffer_list_ptr->buffer_entry[i].sub_buf[j].buf_id = i;
			buffer_list_ptr->buffer_entry[i].sub_buf[j].hcmd_id = INVALID_ID;
			buffer_list_ptr->buffer_entry[i].sub_buf[j].subpage = j;
			buffer_list_ptr->buffer_entry[i].sub_buf[j].dirty = 0;
			buffer_list_ptr->buffer_entry[i].sub_buf[j].using_state = NOT_USING;
			buffer_list_ptr->buffer_entry[i].sub_buf[j].fcl_buf_list_node.prev = NULL;
			buffer_list_ptr->buffer_entry[i].sub_buf[j].fcl_buf_list_node.next = NULL;
			buffer_list_ptr->buffer_entry[i].sub_buf[j].hcmd_buf_list_node.prev = NULL;
			buffer_list_ptr->buffer_entry[i].sub_buf[j].hcmd_buf_list_node.next = NULL;
		}
	}

	// init free buffer list
	for (u32 i = 0; i < BUFFER_NUMBER; i++)
	{
		list_add_tail(&buffer_list_ptr->buffer_entry[i].free_list_node, &free_buf_list_head);
	}

#endif
}

u32 BUFF_check_buf_hit(psa_entry psa, buff_opcode opcode, u32 *buf_id)
{
#if (BYPASS_BUFFER == YES)

	return NOT_HIT;
#else
	buffer_entry *buf;
	u32 addr;
	// check hash table, hash table's entries equal lru list's entries
	if (opcode == BUF_PREREAD)
	{
		addr = psa.addr;
	}
	else
	{
		addr = psa.addr & L2P_MASK;
	}
	buf = search_hash_table_entry(addr);
	u32 subpage = psa.slc_psa.subpage;

	if (buf == NULL)
	{
		//xil_printf("not hello %d \n", lpn);
		return BUF_NOT_HIT;
	}
	else
	{
		if ((buf->opcode == BUF_FREE) || (buf->opcode == BUF_PREREAD_RECLAIM))
		{
			buf->sub_buf[subpage].using_state = USING;
			buf->opcode = opcode;
			*buf_id = buf->buf_id;
			return BUF_HIT;
		}
		else if (buf->opcode == opcode)
		{
			if (buf->sub_buf[subpage].using_state == NOT_USING)
			{
				buf->sub_buf[subpage].using_state = USING;

				*buf_id = buf->buf_id;
				return BUF_HIT;
			}
		}
		*buf_id = INVALID_ID;
		return BUF_OCCUPIED;
	}
#endif
}

u32 BUFF_allocate_buf(psa_entry psa, buff_opcode opcode)
{
#if (BYPASS_BUFFER == YES)
	static u32 ret_buf_id = 0;
	ret_buf_id++;
	ret_buf_id = ret_buf_id % BUFFER_NUMBER;

	return ret_buf_id;
#else
	u32 ret = INVALID_ID;
	buffer_entry *buf = NULL;

	u32 subpage = psa.slc_psa.subpage;
	u32 addr;
	u32 i;

	if (opcode == BUF_PREREAD)
	{
		addr = psa.addr;
	}
	else
	{
		addr = psa.addr & L2P_MASK;
	}

	if (TRUE == list_empty(&free_buf_list_head))
	{
		list_for_each_entry(buf, &lru_buf_list_head, lru_list_node)
		{
			

			if ((buf->opcode == BUF_FREE) || (buf->opcode == BUF_PREREAD_RECLAIM))
			{
				if (buf->hash_table_node.next == NULL)
				{
					assert(0);
				}
				if (buf->hash_table_node.prev == NULL)
				{
					assert(0);
				}
				if ((buf->opcode == BUF_PREREAD_RECLAIM))
				{
					delete_hash_table_entry(buf->psa.addr, buf);
				}
				else
				{
					delete_hash_table_entry((u32)((buf->psa.addr) & L2P_MASK), buf);
				}
				buf->psa.addr = addr;
				insert_hash_table_entry((u32)(addr), buf);
				list_delete(&buf->lru_list_node);
				list_add_head(&buf->lru_list_node, &lru_buf_list_head);
				buf->opcode = opcode;
				buf->sub_buf[subpage].using_state = USING;
				for (i = 0; i < PAGE_SIZE / SUB_PAGE_SIZE; i++)
				{
					buf->sub_buf[i].dirty = BUF_CLEAN;
				}
				return buf->buf_id;
			}
			else
			{
				if ((addr == buf->psa.addr) && (buf->sub_buf[subpage].using_state == NOT_USING) && (opcode == buf->opcode))
				{
					// ?? is this a real situation??
					buf->sub_buf[subpage].using_state = USING;
					return buf->buf_id;
				}
			}
		}
	}
	else
	{
		buf = container_of(free_buf_list_head.next, typeof(*buf), free_list_node);
		list_delete_head(&free_buf_list_head);
		buf->psa.addr = addr;
		buf->opcode = opcode;
		buf->sub_buf[subpage].using_state = USING;
		list_add_head(&buf->lru_list_node, &lru_buf_list_head);
		insert_hash_table_entry(addr, buf);
		return buf->buf_id;
	}
	return ret;
#endif
}

/**********************************************************************************
Func    Name: BUFF_hcmd_get_buffer
Descriptions:
Input   para: buff_id
In&Out  Para: None
Output  para: None
Return value: buffer 
// ***********************************************************************************/
u32 BUFF_get_buffer_addr(u32 buff_id)
{
	u32 buffer_address = buff_id * BUFF_SIZE + BUFFER_BASE_ADDR;
	return buffer_address;
}

u32 BUFF_get_buffer_id(u32 buff_addr)
{
	u32 buffer_id = (buff_addr - BUFFER_BASE_ADDR) / BUFF_SIZE;
	return buffer_id;
}

u32 BUFF_get_hcmd_buff_cnt(u32 hcmd_id)
{
#if (BYPASS_BUFFER == YES)

	return 1;
#else
	return hcmd_buf_table_ptr->cnt[hcmd_id];
#endif
}

void BUFF_hcmd_add_buffer(u32 hcmd_id, u32 buffer_id, u32 subpage)
{
#if (BYPASS_BUFFER == YES)
	static u32 ret_buf_id = 0;
	ret_buf_id++;
	ret_buf_id = ret_buf_id % BUFFER_NUMBER;

#else
	buffer_entry *buf = &buffer_list_ptr->buffer_entry[buffer_id];
	buf->sub_buf[subpage].hcmd_id = hcmd_id;
	list_add_tail(&(buf->sub_buf[subpage].hcmd_buf_list_node), &hcmd_buf_table_ptr->table_entry[hcmd_id]);
	hcmd_buf_table_ptr->cnt[hcmd_id]++;
#endif
}

u32 BUFF_hcmd_get_buffer(u32 hcmd_index, u32 offset, u32 *subpage)
{
#if (BYPASS_BUFFER == YES)
	static u32 ret_buf_id = 0;
	ret_buf_id++;
	ret_buf_id = ret_buf_id % BUFFER_NUMBER;

	return ret_buf_id;

#else
	if (offset >= hcmd_buf_table_ptr->cnt[hcmd_index])
	{
		xil_printf("error!! \n");
	}
	sub_buffer_entry *sub_buf;

	u32 tmp_cnt = 0;

	list_for_each_entry(sub_buf, &hcmd_buf_table_ptr->table_entry[hcmd_index], hcmd_buf_list_node)
	{
		if (offset == tmp_cnt)
		{
			*subpage = sub_buf->subpage;
			return sub_buf->buf_id;
		}
		tmp_cnt++;
	}
#endif
}
boolean buf_is_free(u32 buf_id)
{

	u32 i;
	buffer_entry *buf = &buffer_list_ptr->buffer_entry[buf_id];
	for (i = 0; i < PAGE_SIZE / SUB_PAGE_SZ; i++)
	{
		if (buf->sub_buf[i].using_state != NOT_USING)
		{
#if 0
			if (buf->sub_buf[i].hcmd_id == INVALID_ID)
			{
				buf->sub_buf[i].using_state = NOT_USING;
			}
			else
#endif
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}
void BUFF_hcmd_free_buf(u32 hcmd_index)
{
#if (BYPASS_BUFFER == YES)

#else
	sub_buffer_entry *sub_buf = NULL;
	buffer_entry *buf = NULL;
#if 1
	while (FALSE == list_empty(&hcmd_buf_table_ptr->table_entry[hcmd_index]))
	{
		sub_buf = container_of((&hcmd_buf_table_ptr->table_entry[hcmd_index])->next, typeof(*sub_buf), hcmd_buf_list_node);
		if (sub_buf != NULL)
		{
			sub_buf->using_state = NOT_USING;
			sub_buf->hcmd_id = INVALID_ID;
			buf = &buffer_list_ptr->buffer_entry[sub_buf->buf_id];
			if (buf_is_free(sub_buf->buf_id) == TRUE)
			{
				buf->opcode = BUF_FREE;
			}
		}
		else
		{
			assert(0);
		}
		list_delete(&sub_buf->hcmd_buf_list_node);
		//	list_delete_head(&hcmd_buf_table_ptr->table_entry[hcmd_index]);
	}
#endif
#if 0
	list_for_each_entry(sub_buf, &hcmd_buf_table_ptr->table_entry[hcmd_index], hcmd_buf_list_node)
	{
		if (sub_buf != NULL)
		{
			sub_buf->using_state = NOT_USING;
			sub_buf->hcmd_id = INVALID_ID;
			buf = &buffer_list_ptr->buffer_entry[sub_buf->buf_id];
			if (buf_is_free(sub_buf->buf_id) == TRUE)
			{
				buf->opcode = BUF_FREE;
			}
			list_delete(&sub_buf->hcmd_buf_list_node);
		}
		else
		{
			assert(0);
		}
	}
#endif
	hcmd_buf_table_ptr->cnt[hcmd_index] = 0;
#endif
}

void BUFF_set_buff_psa(u32 buf_id, psa_entry psa)
{
	buffer_entry *buf = &buffer_list_ptr->buffer_entry[buf_id];
	buf->psa.addr = psa.addr;
}

void insert_subbuff_order(sub_buffer_entry *new, list_node *list)
{
	sub_buffer_entry *tmp = NULL;
	buffer_entry *tmp_buf = NULL;
	buffer_entry *new_buf = &buffer_list_ptr->buffer_entry[new->buf_id];
	if (list_empty(list) == TRUE)
	{
		list_add_head(&(new->fcl_buf_list_node), list);
	}
	else
	{
		list_for_each_entry(tmp, list, fcl_buf_list_node)
		{
			if (tmp->buf_id == new->buf_id)
			{
				if (new->subpage < tmp->subpage)
				{
					tmp->fcl_buf_list_node.prev->next = &new->fcl_buf_list_node;
					new->fcl_buf_list_node.prev = tmp->fcl_buf_list_node.prev;
					new->fcl_buf_list_node.next = &tmp->fcl_buf_list_node;
					tmp->fcl_buf_list_node.prev = &new->fcl_buf_list_node;
					return;
				}
			}

			else
			{
				tmp_buf = &buffer_list_ptr->buffer_entry[tmp->buf_id];

				if ((new_buf->psa.addr) < (tmp_buf->psa.addr))
				{
					tmp->fcl_buf_list_node.prev->next = &new->fcl_buf_list_node;
					new->fcl_buf_list_node.prev = tmp->fcl_buf_list_node.prev;
					new->fcl_buf_list_node.next = &tmp->fcl_buf_list_node;
					tmp->fcl_buf_list_node.prev = &new->fcl_buf_list_node;
					return;
				}
			}
		}
		list_add_tail(&new->fcl_buf_list_node, list);
	}
}

void BUFF_get_buff_psa(u32 buffer_id, psa_entry *psa)
{
	buffer_entry *buf = &buffer_list_ptr->buffer_entry[buffer_id];
	psa->addr = buf->psa.addr;
}

void BUFF_node_add_buffer(u32 subpage, u32 buffer_id, buff_opcode op)
{
	u32 ch, ce, lun, plane;
	buffer_entry *buf = &buffer_list_ptr->buffer_entry[buffer_id];
	ch = buf->psa.slc_psa.ch;
	ce = buf->psa.slc_psa.ce;
	lun = buf->psa.slc_psa.lun;
	plane = buf->psa.slc_psa.plane;

	if (op == BUF_READ)
	{
		node_table.node_read_cnt[ch][ce][lun][plane]++;
		insert_subbuff_order(&(buf->sub_buf[subpage]), &(node_table.node_read_list[ch][ce][lun][plane]));
	}
	else
	{
		node_table.node_write_cnt[ch][ce][lun][plane]++;
		insert_subbuff_order(&(buf->sub_buf[subpage]), &(node_table.node_write_list[ch][ce][lun][plane]));
	}
}

u32 BUFF_get_node_cnt(psa_entry psa, buff_opcode op)
{
	u32 ch, ce, lun, plane;

	ch = psa.slc_psa.ch;
	ce = psa.slc_psa.ce;
	lun = psa.slc_psa.lun;
	plane = psa.slc_psa.plane;
	if (op == BUF_READ)
	{
		return node_table.node_read_cnt[ch][ce][lun][plane];
	}
	else
	{
		return node_table.node_write_cnt[ch][ce][lun][plane];
	}
}

void BUFF_dec_node_cnt(psa_entry psa, buff_opcode op, u32 cnt)
{
	u32 ch, ce, lun, plane;

	ch = psa.slc_psa.ch;
	ce = psa.slc_psa.ce;
	lun = psa.slc_psa.lun;
	plane = psa.slc_psa.plane;
	if (op == BUF_READ)
	{
		node_table.node_read_cnt[ch][ce][lun][plane] -= cnt;
	}
	else
	{
		node_table.node_write_cnt[ch][ce][lun][plane] -= cnt;
	}
}

void BUFF_node_de_subbuffer(list_node *node, u32 cnt)
{
	list_node *next_node = NULL;

	while (cnt > 0)
	{

		next_node = node->next;
		list_delete(node);
		//list_delete_head(&(node_table.node_read_list[ch][ce][lun][plane]));
		node = next_node;
		cnt--;
	}
}
u32 BUFF_in_one_page_cnt(psa_entry psa, buff_opcode op)
{
	u32 ch, ce, lun, plane;

	ch = psa.slc_psa.ch;
	ce = psa.slc_psa.ce;
	lun = psa.slc_psa.lun;
	plane = psa.slc_psa.plane;
	sub_buffer_entry *subbuf = NULL;
	sub_buffer_entry *pre_subbuf = NULL;
	buffer_entry *buf = NULL;
	buffer_entry *pre_buf = NULL;
	u32 cnt = 1;
	if (op == BUF_READ)
	{
		list_for_each_entry(subbuf, &(node_table.node_read_list[ch][ce][lun][plane]), fcl_buf_list_node)
		{
			if (subbuf == NULL)
			{
				assert(0);
			}
			if (pre_subbuf != NULL)
			{
				if ((subbuf->buf_id) == (pre_subbuf->buf_id))
				{
					cnt++;
				}
				else
				{
					return cnt;
				}
			}
			pre_subbuf = subbuf;
		}
	}
	else if (op == BUF_WRITE)

	{
		list_for_each_entry(subbuf, &(node_table.node_write_list[ch][ce][lun][plane]), fcl_buf_list_node)
		{
			if (subbuf == NULL)
			{
				assert(0);
			}
			if (pre_subbuf != NULL)
			{
				if ((subbuf->buf_id) == (pre_subbuf->buf_id))
				{
					cnt++;
				}
				else
				{
					return cnt;
				}
			}
			pre_subbuf = subbuf;
		}
	}
	return cnt;
}

buffer_using_state BUFF_get_buff_using_state(u32 buf_id, u32 subpage)
{
	buffer_entry *buf = &buffer_list_ptr->buffer_entry[buf_id];
	return buf->sub_buf[subpage].using_state;
}

void BUFF_set_buff_using_state(u32 buf_id, buffer_using_state using_state, u32 subpage)
{
	buffer_entry *buf = &buffer_list_ptr->buffer_entry[buf_id];
	buf->sub_buf[subpage].using_state = using_state;
}

void BUFF_set_subpage_dirty(u32 buf_id, u32 subpage)
{
	buffer_entry *buf = &buffer_list_ptr->buffer_entry[buf_id];
	buf->sub_buf[subpage].dirty = BUF_DIRTY;
}

void BUFF_set_subpage_hcmdid(u32 buf_id, u32 subpage, u32 hcmd_id)
{
	buffer_entry *buf = &buffer_list_ptr->buffer_entry[buf_id];
	buf->sub_buf[subpage].hcmd_id = hcmd_id;
}

void BUF_preread_done(u32 buf_id)
{
	buffer_entry *preread_buf;
	psa_entry preread_psa;
	buffer_entry *buf;
	psa_entry psa;
	u32 addr;
	u32 pre_bufaddr, bufaddr;
	u32 write_subpage, i, j, cnt;
	preread_buf = &(buffer_list_ptr->buffer_entry[buf_id]);
	preread_psa.addr = preread_buf->psa.addr;
	addr = preread_psa.addr & L2P_MASK;
	buf = search_hash_table_entry(addr);
	if (buf == NULL)
	{
		assert(!("buf null in BUF_preread_done!!!\n"));
	}

	pre_bufaddr = BUFF_get_buffer_addr(preread_buf->buf_id);
	bufaddr = BUFF_get_buffer_addr(buf->buf_id);
	write_subpage = preread_psa.addr & (~L2P_MASK);
	write_subpage = (write_subpage & 0x3) | ((write_subpage >> 28) & 0xc);
	i = 1;
	j = 0;
	while (j < PAGE_SIZE / SUB_PAGE_SZ)
	{
		if (buf->sub_buf[j].using_state != NOT_USING)
		{
			if (write_subpage & i == 0)
			{
				memcpy((void *)(bufaddr + j * SUB_PAGE_SIZE), (void *)(pre_bufaddr + j * SUB_PAGE_SIZE), SUB_PAGE_SIZE);
				BUFF_set_subpage_dirty(buf->buf_id, j);
			}
		}
		j++;
		i = i << 1;
	}
	buf->opcode = BUF_PREREAD_DONE;
	preread_buf->opcode = BUF_PREREAD_RECLAIM;

	preread_buf->sub_buf[preread_psa.slc_psa.subpage].using_state = NOT_USING;
	preread_buf->sub_buf[preread_psa.slc_psa.subpage].hcmd_id = INVALID_ID;
}

psa_entry BUFF_cal_preread_psa(psa_entry psa, u32 cnt)
{
	u32 subpage;
	u32 addr;
	subpage = psa.slc_psa.subpage;
	addr = psa.addr & L2P_MASK;

	while (cnt > 0)
	{
		if (subpage > 1)
		{
			addr = addr | (1 << (subpage + 28));
		}
		else
		{
			addr = addr | (1 << subpage);
		}
		cnt--;
		subpage++;
	}
	return (psa_entry)addr;
}

u32 BUFF_get_hcmd_id(u32 buf_id, u32 subpage)
{
	buffer_entry *buf = &(buffer_list_ptr->buffer_entry[buf_id]);
	if (buf == NULL)
	{
		assert(0);
	}
	if (buf->sub_buf[subpage].using_state != NOT_USING)
	{
		return buf->sub_buf[subpage].hcmd_id;
	}
	else
	{
		return INVALID_ID;
	}
}

boolean BUFF_in_one_page_all_dirty(u32 buf_id)
{
	buffer_entry *buf = &buffer_list_ptr->buffer_entry[buf_id];
	u32 i;
	for (i = 0; i < PAGE_SIZE / SUB_PAGE_SZ; i++)
	{
		if (buf->sub_buf[i].dirty != BUF_DIRTY)
		{
			return FALSE;
		}
	}
	return TRUE;
}

#if 0
void BUFF_unit_test()
{
	EMU_log_println(LOG, "begin buffer test");
	BUFF_init_buffer();
	buffer_list_ptr = (buffer_list *)(BUFFER_ENTRY_BASE_ADDR);
	psa_entry e;
	L2P_set_psa(&e, 0, 1, 0, 1, 3, 4, 1);
	// buffer_entry *be = &buffer_list_ptr->buffer_entry[0];
	// L2P_set_psa(&be->psa, 0, 1, 0, 1, 3, 4 ,1);
	// be->psa.slc_psa.dirty = ~(be->psa.slc_psa.dirty);
	// insert_hash_table_entry(e.addr & L2P_MASK, be);
	// insert_hash_table_entry(e.addr & L2P_MASK, &buffer_list_ptr->buffer_entry[1]);

	// delete_hash_table_entry(e.addr & L2P_MASK);
	// search_hash_table_entry(e.addr & L2P_MASK);

	for (u32 i = 0; i < 200; i++)
	{
		buffer_entry *be = &buffer_list_ptr->buffer_entry[i];
		if (i % 10 == 3 || i % 10 == 7 || i % 10 == 9)
		{
			e.addr = 10 & L2P_MASK;
			be->psa.addr = 10 & L2P_MASK;
		}
		else
		{
			e.addr = (10 + HASH_TABLE_SIZE) & L2P_MASK;
			be->psa.addr = (10 + HASH_TABLE_SIZE) & L2P_MASK;
		}
		be->psa.slc_psa.dirty = ~(be->psa.slc_psa.dirty);
		be->psa.slc_psa.valid = ~(be->psa.slc_psa.valid);
		if (i % 3 == 2)
		{
			delete_hash_table_entry(e.addr & L2P_MASK);
		}
		else
		{
			insert_hash_table_entry(e.addr & L2P_MASK, &buffer_list_ptr->buffer_entry[i]);
		}
	}

	exit(0);
}

void BUFF_unit_test1()
{
	u32 hcmd_id, buffer_id, subpage, offset, cnt, i;
	psa_entry e;
	u32 test_num = 6;
	EMU_log_println(LOG, "begin buffer test1");
	BUFF_init_buffer();
	buffer_id = 0;
	for (hcmd_id = 0; hcmd_id < test_num; hcmd_id++)
	{
		for (i = 0; i < 9; i++)
		{
			buffer_id = buffer_id + i;
			for (subpage = 0; subpage < PAGE_SIZE / SUB_PAGE_SIZE; subpage++)
			{

				BUFF_hcmd_add_buffer(hcmd_id, buffer_id, subpage);
			}
		}
		buffer_id++;
	}
	for (hcmd_id = 0; hcmd_id < test_num; hcmd_id++)
	{
		cnt = BUFF_get_hcmd_buff_cnt(hcmd_id);
		EMU_log_println(LOG, "cnt %d ", cnt);

		for (offset = 0; offset < cnt; offset++)
		{
			buffer_id = BUFF_hcmd_get_buffer(hcmd_id, offset, &subpage);
			EMU_log_println(LOG, "buffid %d,subpage %d hcmdid %d\n", buffer_id, subpage, hcmd_id);
		}
	}
	for (hcmd_id = 0; hcmd_id < test_num; hcmd_id++)
	{
		BUFF_hcmd_free_buf(hcmd_id);
		cnt = BUFF_get_hcmd_buff_cnt(hcmd_id);
		if (cnt != 0)
		{
			EMU_log_println(LOG, "cnt %d,hcmdid %d\n ", cnt, hcmd_id);
		}
	}
	exit(0);
}

void BUFF_unit_test2()
{
	u32 i, buffer_id, subpage, check_id, test_num;
	buff_opcode opcode;
	psa_entry psa;
	u32 ch, ce, lun, plane, block, page;
	u32 ret;
	EMU_log_println(LOG, "\n\nbegin buffer test2");
	BUFF_init_buffer();
	psa.addr = 0;
	test_num = 100;
	for (i = 0; i < test_num; i++)
	{

		opcode = BUF_WRITE;
		buffer_id = BUFF_allocate_buf(psa, opcode);
		ret = BUFF_check_buf_hit(psa, opcode, &check_id);
		EMU_log_println(LOG, "buffid %d \n", buffer_id);
		if (ret != BUF_OCCUPIED)
		{
			EMU_log_println(ERR, "should be hit buff id %d,write,psa %d,ret %d \n", buffer_id, psa, ret);
		}
		psa.addr += 8;
	}
	exit(0);
}

void BUFF_unit_test3()
{
	psa_entry psa, tmpsa;
	u32 buffer_id, subpage;
	buff_opcode op;
	u32 cnt, test_num, ch, ce, lun, plane;
	psa.addr = 0;
	test_num = 6;
	op = BUF_WRITE;
	BUFF_init_buffer();
	for (buffer_id = 0; buffer_id < test_num; buffer_id++)
	{
		BUFF_set_buff_psa(buffer_id, psa);
		for (subpage = 0; subpage < (PAGE_SIZE / SUB_PAGE_SZ); subpage++, psa.addr++)
		{
			BUFF_node_add_buffer(subpage, buffer_id, op);
		}
		BUFF_get_buff_psa(buffer_id, &tmpsa);
		if (tmpsa.addr != (psa.addr - 1) & L2P_MASK)
		{
			EMU_log_println(ERR, "buf id %d,tmpsa.addr %d ,psa.addr %d\n", buffer_id, tmpsa.addr, psa.addr);
		}
		psa.addr--;
		cnt = BUFF_in_one_page_cnt(psa, op);
		psa.addr++;
		if (cnt != PAGE_SIZE / SUB_PAGE_SZ)
		{
			EMU_log_println(ERR, "ch %d,ce %d,lun %d,plane %d,cnt %d\n", psa.slc_psa.ch, psa.slc_psa.ce, psa.slc_psa.lun, psa.slc_psa.plane, cnt);
		}
	}
	EMU_log_println(LOG, "APART\n");
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

					cnt = BUFF_get_node_cnt(psa, op);
					BUFF_dec_node_cnt(psa, op, cnt);
					BUFF_node_de_subbuffer(psa, op, cnt);
					cnt = BUFF_get_node_cnt(psa, op);
					if (cnt != 0)
					{
						EMU_log_println(ERR, "cnt=!0,ch %d,ce %d,lun %d,plane %d,cnt %d\n", ch, ce, lun, plane, cnt);
					}
				}
			}
		}
	}
	exit(0);
}
#endif