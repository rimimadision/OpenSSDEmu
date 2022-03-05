#include "lock.h"

#include "../config/mem.h"

#include "../emu/emu_config.h"

u32 *my_lock;

/**********************************************************************************
Func    Name: init_spin_lock
Descriptions: 閸掓繂顫愰崠鏍殰閺冨鏀�
Input   para: g_spin_lock
In&Out  Para: None
Output  para: None
Return value: None
***********************************************************************************/
inline void init_spin_lock(u32 *g_spin_lock)
{

    *g_spin_lock = 0;
}

/**********************************************************************************
Func    Name: get_spin_lock
Descriptions: 閹绘劒绶甸崚婵嗩潗閸栨牜娈戦崙鑺ユ殶
Input   para: g_spin_lock
In&Out  Para: None
Output  para: None
Return value: None
***********************************************************************************/
inline void get_spin_lock(u32 *g_spin_lock)
{
#ifndef EMU
    u32 temp = 0;
    __asm__ __volatile__(
        "1: ldrex  %0, [%1] \n"
        "teq   %0, #0 \n"
        "strexeq   %0, %2, [%1] \n"
        "teqeq   %0, #0 \n"
        "bne   1b \n"
        : "=&r"(temp)
        : "r"(g_spin_lock), "r"(1)
        : "cc");
    asm("isb");
#endif
}

/**********************************************************************************
Func    Name: release_spin_lock
Descriptions: 闁插﹥鏂侀懛顏呮闁匡拷
Input   para: g_spin_lock
In&Out  Para: None
Output  para: None
Return value: None
***********************************************************************************/
inline void release_spin_lock(u32 *g_spin_lock)
{
#ifndef EMU
    //__asm__ __volatile__("" : : : "memory");
    *g_spin_lock = 0;
    __asm__ __volatile__(
        "nop \n"
        "nop \n"
        "nop \n"
        "nop \n"
        "nop");
    asm("isb");
#endif
}

/**********************************************************************************
Func    Name: init_all_spin_lock
Descriptions: 鍒濆鍖栬兘鐢ㄥ埌鐨勬墍鏈塻pin lock
Input   para: None
In&Out  Para: None
Output  para: None
Return value: None
***********************************************************************************/
void init_all_spin_lock()
{
    my_lock = (u32 *)GOBAL_SPIN_LOCK_1_ADDR;

    init_spin_lock((u32 *)GOBAL_SPIN_LOCK_1_ADDR);

    init_spin_lock((u32 *)CH0_SQ_SPIN_LOCK);
    init_spin_lock((u32 *)CH1_SQ_SPIN_LOCK);
    init_spin_lock((u32 *)CH2_SQ_SPIN_LOCK);
    init_spin_lock((u32 *)CH3_SQ_SPIN_LOCK);

    init_spin_lock((u32 *)TQ_FROMCQ_SPIN_LOCK);

    init_spin_lock((u32 *)TO_DATAMOVED_SPIN_LOCK);
    init_spin_lock((u32 *)TO_FINISH_SPIN_LOCK);

    init_spin_lock((u32 *)HCMD_MAP_SPIN_LOCK);

    init_spin_lock((u32 *)SYN_ERASE_SPIN_LOCK);
}
