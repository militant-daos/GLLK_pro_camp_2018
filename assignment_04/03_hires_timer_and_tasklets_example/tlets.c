#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#define HRT_UPDATE_INTERVAL_MS (1000)
#define HRT_STOP_MAX_ATTEMPTS (10)
#define HRT_DELAY_BETWEN_STOP_ATTEMPTS_MS (100)

#define MS_TO_NS(x) ((x) * 1E6L)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Victor Krasnoshchok");
MODULE_DESCRIPTION("HRT example");
MODULE_VERSION("0.1");

static struct tasklet_struct tasklet_norm;
static struct tasklet_struct tasklet_norm1;
static struct tasklet_struct tasklet_norm2;
static struct tasklet_struct tasklet_norm3;
static struct tasklet_struct tasklet_hi;

static struct hrtimer timer_obj;

static void tl_norm_callback(unsigned long data)
{
    printk(KERN_INFO "TASKLET-NR-%lu\n", data);
}

static void tl_hi_callback(unsigned long data)
{
    (void) data;
    printk(KERN_INFO "TASKLET-HI\n");
}

void schedule_tasklets(void)
{
    tasklet_schedule(&tasklet_norm);
    tasklet_schedule(&tasklet_norm1);
    tasklet_schedule(&tasklet_norm2);
    tasklet_schedule(&tasklet_norm3);
    tasklet_hi_schedule(&tasklet_hi);
}

/* Timer-related stuff */

static enum hrtimer_restart timer_common_handler(struct hrtimer* timer_stc)
{
    hrtimer_forward_now(timer_stc, MS_TO_NS(HRT_UPDATE_INTERVAL_MS));
    return HRTIMER_RESTART;
}

static enum hrtimer_restart timer_callback(struct hrtimer* timer)
{
    schedule_tasklets();
    printk(KERN_INFO "---------------\n");
    return timer_common_handler(timer);
}

static int __init hrt_init(void)
{
    ktime_t test_ktime;
    
    tasklet_init(&tasklet_norm, tl_norm_callback, 0); 
    tasklet_init(&tasklet_norm1, tl_norm_callback, 1); 
    tasklet_init(&tasklet_norm2, tl_norm_callback, 2); 
    tasklet_init(&tasklet_norm3, tl_norm_callback, 3); 
    tasklet_init(&tasklet_hi, tl_hi_callback, 0);
    
    test_ktime = ktime_set(0, MS_TO_NS(HRT_UPDATE_INTERVAL_MS));
    
    hrtimer_init(&timer_obj, CLOCK_REALTIME, HRTIMER_MODE_REL);
    timer_obj.function = &timer_callback;
    
    hrtimer_start(&timer_obj, test_ktime, HRTIMER_MODE_REL);
    
    printk(KERN_INFO "TL timer module loaded.\n");
    return 0;
}

static void __exit hrt_exit(void)
{
    int stop_attempts = 0;

    while ((-1 == hrtimer_try_to_cancel(&timer_obj)) &&
           (stop_attempts < HRT_STOP_MAX_ATTEMPTS))
    {
        ++stop_attempts;
        msleep(HRT_DELAY_BETWEN_STOP_ATTEMPTS_MS);
    }

    if (stop_attempts >= HRT_STOP_MAX_ATTEMPTS)
    {
        printk(KERN_ERR "Failed to stop the HR timer.\n");
    }

    tasklet_kill(&tasklet_norm);
    tasklet_kill(&tasklet_norm1);
    tasklet_kill(&tasklet_norm2);
    tasklet_kill(&tasklet_norm3);
    tasklet_kill(&tasklet_hi);

    printk(KERN_INFO "HR timer module unloaded.\n");
}

module_init(hrt_init);
module_exit(hrt_exit);
