#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>

#define WQ_UPDATE_INTERVAL (1 * HZ)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Victor Krasnoshchok");
MODULE_DESCRIPTION("Deferred WQ example");
MODULE_VERSION("0.1");

static struct workqueue_struct* timed_wq;
static struct delayed_work timed_work;

void schedule_work_item(void)
{
    if (false == schedule_delayed_work(&timed_work, WQ_UPDATE_INTERVAL))
    {
        printk(KERN_ERR "Failed to schedule a work item. Already scheduled?\n");
    }
}

static void timed_wq_callback(struct work_struct* work)
{
    struct timespec time_raw;

    getnstimeofday(&time_raw);
    printk(KERN_INFO "HRT: %lu\n", time_raw.tv_sec);
    schedule_work_item();
}

static int __init dfwq_init(void)
{
    timed_wq = create_singlethread_workqueue("DF_WQ");

    if (NULL == timed_wq)
    {
        printk(KERN_ERR "Failed to create the workqueue.\n");
        return -1;
    }
        
    INIT_DELAYED_WORK(&timed_work, timed_wq_callback);
    schedule_work_item();

    printk(KERN_INFO "DF_WQ timer module loaded.\n");
    return 0;
}

static void __exit dfwq_exit(void)
{
    if (NULL != timed_wq)
    {
        cancel_delayed_work_sync(&timed_work);
        flush_workqueue(timed_wq);
        destroy_workqueue(timed_wq);
    }

    printk(KERN_INFO "DF_WQ timer module unloaded.\n");
}

module_init(dfwq_init);
module_exit(dfwq_exit);
