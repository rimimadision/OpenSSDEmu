#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <stddef.h>

#include "fe.h"
#include "shmem.h"
#include "../emu_log.h"
#include "../../lib/list.h"

u32 shm_base;
sem_t *sem_id;

void fe_init()
{
    /* ========== init shm ========== */
    int shm_fd;

    /* if SHM already exist, then delete it */
    if(shm_open(SHM_NAME, O_RDONLY, 0666) != -1)
    {
        shm_unlink(SHM_NAME);
    }

    /* create the shared memory segment as if it was a file */
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        emu_log_println(ERR, "Shared memory failed");
        shm_unlink(SHM_NAME);
        exit(1);
    }

    /* configure the size of the shared memory segment */
    if (ftruncate(shm_fd, SHM_SIZE) == -1)
    {
        emu_log_println(ERR, "Ftruncate failed");
        shm_unlink(SHM_NAME);
        exit(1);
    }

    /* map the shared memory segment to the address space of the process */
    shm_base = (u32)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if ((void *)shm_base == MAP_FAILED)
    {
        emu_log_println(ERR, "Map failed");
        munmap((void *)shm_base, SHM_SIZE);
        shm_unlink(SHM_NAME);
        exit(1);
    }

    /* initialize all 0 */
    memset((void *)SHM_BASE, 0, SHM_SIZE);

    /* init list heads in shm */
    FREE_LIST_HEAD->prev = FREE_LIST_HEAD->next = CMD_SLOT_NUM;
    RDY_LIST_HEAD->prev = RDY_LIST_HEAD->next = CMD_SLOT_NUM;
    PROC_LIST_HEAD->prev = PROC_LIST_HEAD->next = CMD_SLOT_NUM;

    /* add all slot into free_list */
    for (int i = 0; i < CMD_SLOT_NUM; i++)
    {
        shm_list_add(i, FREE_LIST);
    }
    // munmap(shm_base, SHM_SIZE);
    // shm_unlink(SHM_NAME);

    /* ========== init semaphore to synchronize fio and emu ========== */
    // sem_unlink(SEM_NAME);
    sem_id = sem_open(SEM_NAME, O_CREAT | O_RDWR, 0666, 1);

    if (sem_id == SEM_FAILED)
    {
        emu_log_println(ERR, "sem_open Failed");
        sem_unlink(SEM_NAME);
        exit(1);
    }

    /* Add one test cmd */
    shm_index t_i = shm_list_remove(FREE_LIST);
    shm_cmd *t_cmd = SHM_SLOT(t_i);
    t_cmd->lpn = 0;
    t_cmd->ops = SHM_WRITE_OPS;
    t_cmd->size = 16 * KB;
    shm_list_add(t_i, RDY_LIST);


    // int v = 0;sem_getvalue(sem_id, &v);
	//     printf("%d\n", v);

    // while(1);
    // emu_log_println(LOG, "value : %d", v);
    // sem_close(sem_id);
    // sem_unlink(SEM_NAME);

    // emu_log_println(LOG, "shmbase : %x", SHM_BASE);
    // emu_log_println(LOG, "shmDATAbase : %x", SHM_DATA_BASE);
    // emu_log_println(LOG, "shmcmdbase : %x", SHM_CMD_BASE);
    // emu_log_println(LOG, "free : %x", FREE_LIST_HEAD);
    // emu_log_println(LOG, "rdy : %x", RDY_LIST_HEAD );
    // emu_log_println(LOG, "proc : %x", PROC_LIST_HEAD );
    // int count = 0;
    // while (1)
    // {
    //     usleep(1000);
    //     sem_wait(sem_id);
    //     if (shm_list_empty(RDY_LIST))
    //     {
    //         sem_post(sem_id);
    //         continue;
    //     }
    //     shm_index i = shm_list_remove(RDY_LIST);
    //     shm_list_add(i, FREE_LIST);
    //     emu_log_println(LOG, "%d", SHM_SLOT(i)->lpn);
    //     // emu_log_println(LOG, "count %d KB", (count += 4));
    //     sem_post(sem_id);
    // }
}

void *fe()
{
    emu_log_println(LOG, "Fe start");

    while (1)
    {
        /* code */
    }
    

}