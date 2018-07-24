#include <linux/module.h>
#include <linux/kthread.h>

MODULE_AUTHOR("Victor Krasnoshchok");
MODULE_DESCRIPTION("Kthreads sync. sample");
MODULE_LICENSE("GPL");

#define WORKERS_COUNT (4)
#define SHARED_VAR_UPDATE_INT_S (10 * HZ)

struct worker_struct_s {
	struct task_struct *worker_thread;
	unsigned int       thread_id;
	struct completion  comp_obj;
};

static struct worker_struct_s workers[WORKERS_COUNT];

/*
 * "Volatile" specifier is used here just to show
 * synchronization using not an atomic operations
 * but more "traditional" mutex obj.
 *
 * checkpatch.pl will complain here.
 */
static volatile unsigned int shared_var;

static DEFINE_MUTEX(sh_var_access_mtx);

static int worker_proc(void *arg)
{
	struct worker_struct_s *self = (struct worker_struct_s *) arg;
	unsigned long local_copy = 0;

	set_current_state(TASK_INTERRUPTIBLE);

	while (!kthread_should_stop()) {
		mutex_lock(&sh_var_access_mtx);
		local_copy = ++shared_var;
		mutex_unlock(&sh_var_access_mtx);

		pr_info("Thread %d : %lu\n", self->thread_id, local_copy);

		schedule_timeout_interruptible(SHARED_VAR_UPDATE_INT_S);
	}

	complete(&self->comp_obj);
	pr_info("Thread %d: completed.\n", self->thread_id);
	return 0;
}

static void threads_pool_cleanup(void)
{
	int i;

	for (i = 0; i < WORKERS_COUNT; ++i) {
		if (!IS_ERR(workers[i].worker_thread)) {
			kthread_stop(workers[i].worker_thread);
			wait_for_completion_interruptible(&workers[i].comp_obj);
		}
	}

	pr_info("Threads pool cleanup done.\n");
}

static int __init sync_sample_init(void)
{
	int i;

	for (i = 0; i < WORKERS_COUNT; ++i) {
		init_completion(&workers[i].comp_obj);
		workers[i].thread_id = i;

		workers[i].worker_thread =
			kthread_run(worker_proc,
				(void *) &workers[i],
				"worker-%d", i);

		if (IS_ERR(workers[i].worker_thread)) {
			pr_err("Failed to instantiate thread %d.\n", i);
			goto err_init_cleanup;
		}

		pr_info("Thread %d created.\n", i);
	}

	return 0;

err_init_cleanup:
	threads_pool_cleanup();
	return -1;
}

static void __exit sync_sample_exit(void)
{
	threads_pool_cleanup();
}

module_init(sync_sample_init);
module_exit(sync_sample_exit);
