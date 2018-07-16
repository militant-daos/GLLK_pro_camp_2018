#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

#define HRT_UPDATE_INTERVAL_MS (1000)
#define HRT_STOP_MAX_ATTEMPTS (10)
#define HRT_DELAY_BETWEN_STOP_ATTEMPTS_MS (100)

#define MS_TO_NS(x) ((x) * 1E6L)

typedef struct HRT_timer_desc_STCT
{
    struct hrtimer timer_obj;
    bool   need_to_restart;
} HRT_timer_desc_STC;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Victor Krasnoshchok");
MODULE_DESCRIPTION("HRT example");
MODULE_VERSION("0.1");

static HRT_timer_desc_STC hrt_obj = {0};
static struct workqueue_struct* timed_wq;
static struct work_struct timed_work;

static void timed_wq_callback(struct work_struct* work)
{
    struct timespec time_raw;

    getnstimeofday(&time_raw);
    printk(KERN_INFO "HRT: %lu\n", time_raw.tv_sec);
}

/* Timer-related stuff */

static enum hrtimer_restart timer_common_handler(HRT_timer_desc_STC* timer_stc)
{
    if (true == timer_stc->need_to_restart)
    {
        hrtimer_forward_now(&(timer_stc->timer_obj), MS_TO_NS(HRT_UPDATE_INTERVAL_MS));
        return HRTIMER_RESTART;
    }

    return HRTIMER_NORESTART;
}

static enum hrtimer_restart timer_callback(struct hrtimer* timer)
{
    HRT_timer_desc_STC* timer_stc = (HRT_timer_desc_STC*) timer;
    
    if (false == queue_work(timed_wq, &timed_work))
    {
        timer_stc->need_to_restart = false;
        printk(KERN_ERR "The work was already in the queue! How this can be?! Wait! Oh sh...\n");
    }
    
    return timer_common_handler(timer_stc);
}

static int __init hrt_init(void)
{
    ktime_t test_ktime;

    timed_wq = create_singlethread_workqueue("HRT_WQ");

    if (NULL == timed_wq)
    {
        printk(KERN_ERR "Failed to create the workqueue.\n");
        return -1;
    }
        
    INIT_WORK(&timed_work, timed_wq_callback);
    
    test_ktime = ktime_set(0, MS_TO_NS(HRT_UPDATE_INTERVAL_MS));
    
    hrtimer_init((struct hrtimer*) &hrt_obj, CLOCK_REALTIME, HRTIMER_MODE_REL);
    hrt_obj.timer_obj.function = &timer_callback;
    hrt_obj.need_to_restart = true;
    
    hrtimer_start((struct hrtimer*) &hrt_obj, test_ktime, HRTIMER_MODE_REL);
    
    printk(KERN_INFO "HR timer module loaded.\n");
    return 0;
}

static void __exit hrt_exit(void)
{
    int stop_attempts = 0;

    if (NULL != timed_wq)
    {

        hrt_obj.need_to_restart = false;

        while ((-1 == hrtimer_try_to_cancel(&hrt_obj.timer_obj)) &&
               (stop_attempts < HRT_STOP_MAX_ATTEMPTS))
        {
            ++stop_attempts;
            msleep(HRT_DELAY_BETWEN_STOP_ATTEMPTS_MS);
        }

        if (stop_attempts >= HRT_STOP_MAX_ATTEMPTS)
        {
            printk(KERN_ERR "Failed to stop the HR timer.\n");
        }

        flush_workqueue(timed_wq);
        destroy_workqueue(timed_wq);
    }

    printk(KERN_INFO "HR timer module unloaded.\n");
}

module_init(hrt_init);
module_exit(hrt_exit);
