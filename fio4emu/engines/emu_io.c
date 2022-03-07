/*
 * SSDEmu engine
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include <stdlib.h>

#include "../../emu/fe/shmem.h"
#include "../fio.h"

pthread_mutex_t mu;
sem_t *sem_id = NULL;

struct fio_emussd_data
{
	int shm_fd;
	void *shm_base;
};

// TODO: send buf data to shm when write
void send_write_cmd_to_shm(unsigned long long offset, void *data_buf, unsigned long long data_len, __u64 shm_base)
{
	shm_index i;
	shm_cmd *head_slot;
	int lpn = offset / HOST_BLOCK_SIZE;

	i = shm_list_remove_x64(shm_base, FREE_LIST);
	log_info("remove %d lpn %llu\n", i, lpn);
	head_slot = SHM_SLOT(i);
	assert(head_slot);
	assert(i <= 255 && i >= 0);
	head_slot->lpn = lpn;
	head_slot->ops = SHM_WRITE_OPS;
	head_slot->size = data_len;
	shm_list_add_x64(shm_base, i, RDY_LIST);  
}

void send_read_cmd_to_shm(unsigned long long offset, void *data_buf, unsigned long long data_len, __u64 shm_base)
{
	shm_index i;
	shm_cmd *head_slot;
	int lpn = offset / HOST_BLOCK_SIZE;

	i = shm_list_remove_x64(shm_base, FREE_LIST);
	log_info("remove %d lpn %llu\n", i, lpn);
	head_slot = SHM_SLOT(i);
	assert(head_slot);
	assert(i <= 255 && i >= 0);
	head_slot->lpn = lpn;
	head_slot->ops = SHM_READ_OPS;
	head_slot->size = data_len;
	shm_list_add_x64(shm_base, i, RDY_LIST);
}

static enum fio_q_status fio_emussd_queue(struct thread_data *td,
										  struct io_u *io_u)
{
	struct fio_file *f;
	struct fio_emussd_data *fed;
	__u64 shm_base;

	pthread_mutex_lock(&mu);
	f = io_u->file;
	fed = FILE_ENG_DATA(f);
	shm_base = (__u64)fed->shm_base;

	fio_ro_check(td, io_u);
	shm_get(shm_base);

	/*
	 * check if free space for cmd
	 * may get a lot competion here
	 */
	while (shm_list_empty_x64(shm_base, FREE_LIST))
	{
		shm_release(shm_base);
		usleep(10);
		shm_get(shm_base);
	}
	log_info("[%ld] ", td->thread);
	/* begin read and write */
	if (io_u->ddir == DDIR_READ)
	{
		send_read_cmd_to_shm(io_u->offset, io_u->xfer_buf, io_u->xfer_buflen, shm_base);
	}
	else if (io_u->ddir == DDIR_WRITE)
	{
		send_write_cmd_to_shm(io_u->offset, io_u->xfer_buf, io_u->xfer_buflen, shm_base);
	}
	else if (io_u->ddir == DDIR_TRIM)
	{
		log_info("shouldn't trim\n");
		do_io_u_trim(td, io_u);
		return FIO_Q_COMPLETED;
	}
	else
	{
		log_info("should have ddir\n");
		do_io_u_sync(td, io_u);
	}
	pthread_mutex_unlock(&mu);
	shm_release(shm_base);

	return FIO_Q_COMPLETED;
}

static int fio_emussd_open_file(struct thread_data *td, struct fio_file *f)
{
	struct fio_emussd_data *fed;
	pthread_mutex_lock(&mu);

	fed = (struct fio_emussd_data *)calloc(1, sizeof(struct fio_emussd_data));
	// if (sem_id == NULL)
	// {
	// 	sem_id = sem_open(SEM_NAME, O_RDWR);
	// 	sem_wait(sem_id);
	// 	if (sem_id == SEM_FAILED)
	// 	{
	// 		log_err("sem_open Failed\n");
	// 		sem_unlink(SEM_NAME);
	// 		return 1;
	// 	}
	// 	int v = 0;
	// 	sem_getvalue(sem_id, &v);
	// 	printf("sem : %d\n", v);
	// }

	/* create the shared memory segment as if it was a file */
	fed->shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
	if (fed->shm_fd == -1)
	{
		log_err("Shared memory failed\n");
		shm_unlink(SHM_NAME);
		return 1;
	}

	/* configure the size of the shared memory segment */
	if (ftruncate(fed->shm_fd, SHM_SIZE) == -1)
	{
		log_err("Ftruncate failed\n");
		shm_unlink(SHM_NAME);
		return 1;
	}

	/* map the shared memory segment to the address space of the process */
	fed->shm_base = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fed->shm_fd, 0);
	if (fed->shm_base == MAP_FAILED)
	{
		log_err("Map failed\n");
		munmap(fed->shm_base, SHM_SIZE);
		shm_unlink(SHM_NAME);
		return 1;
	}

	FILE_SET_ENG_DATA(f, fed);

	pthread_mutex_unlock(&mu);
	return 0;
}

static int fio_emussd_close_file(struct thread_data *td, struct fio_file *f)
{
	int ret = 0;
	struct fio_emussd_data *fed;
	fed = (struct fio_emussd_data *)FILE_ENG_DATA(f);

	pthread_mutex_lock(&mu);
	// sem_wait(sem_id);
	FILE_SET_ENG_DATA(f, NULL);
	free(fed);
	fio_file_clear_partial_mmap(f);
	ret = generic_close_file(td, f);
	// sem_post(sem_id);
	// sem_close(sem_id);
	pthread_mutex_unlock(&mu);

	return ret;
}

static int get_emussd_size(struct thread_data *td, struct fio_file *f)
{
	f->real_file_size = (uint64_t)(EMU_SSD_SIZE);
	return 0;
}

static struct ioengine_ops ioengine = {
	.name = "emu_io",
	.version = FIO_IOOPS_VERSION,
	.queue = fio_emussd_queue,
	.open_file = fio_emussd_open_file,
	.close_file = fio_emussd_close_file,
	.get_file_size = get_emussd_size,
	.flags = FIO_SYNCIO | FIO_NOEXTEND,
};

static void fio_init fio_emussd_register(void)
{
	register_ioengine(&ioengine);
}

static void fio_exit fio_emussd_unregister(void)
{
	unregister_ioengine(&ioengine);
}
