#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Victor Krasnoshchok");
MODULE_DESCRIPTION("Blink a LED");
MODULE_VERSION("0.2");

/* Misc */
typedef volatile u32* reg_ptr_t;
#define BYTE_PTR(x) (x/sizeof(u32))
#define DEFAULT_IOREMAP_LEN (4096)

/* GPIO muxing & data mapping */
#define GPIO1_MAP_BASE (0x4804c000)
#define GPIO1_OE_REG (BYTE_PTR(0x134))

#define GPIO1_DATA_OUT_SET (BYTE_PTR(0x194))
#define GPIO1_DATA_OUT_CLR (BYTE_PTR(0x190))

/* GPIO muxing constants */
#define GPIO_OE_OUT (0)
#define GPIO_PIN_HIGH (1)

/* GPIO LED pins */
/* On-board USR3 LED */
#define GPIO_LED_USR3_PIN (24)

#define MAX_LED_BLINK_TIMES (10)
#define LED_LIGHT_INTERVAL_SEC (1 * HZ)

/* **************************************** */

/* Regs mappings */
static void __iomem* gpio1_base;

/* LED object and timer abstraction */
typedef struct LED_control_STCT
{

    struct timer_list led_timer;
    unsigned long     led_blink_interval; 
    unsigned int      led_gpio_pin;
    unsigned int      led_curr_blink_times;
    unsigned int      led_max_blink_times;
    bool              is_led_on;

} LED_control_STC;

LED_control_STC led_usr3_ctl = {
        .led_blink_interval   = LED_LIGHT_INTERVAL_SEC,
        .led_gpio_pin         = GPIO_LED_USR3_PIN,
        .led_curr_blink_times = 0,
        .led_max_blink_times  = MAX_LED_BLINK_TIMES,
        .is_led_on            = false
};

/* **************************************** */

/* GPIO helpers */
static void switch_led_on(LED_control_STC* led_ctl_ptr)
{
       *((reg_ptr_t) gpio1_base + GPIO1_DATA_OUT_SET) |= (GPIO_PIN_HIGH << (led_ctl_ptr->led_gpio_pin));
       led_ctl_ptr->is_led_on = true;
}

static void switch_led_off(LED_control_STC* led_ctl_ptr)
{
       *((reg_ptr_t) gpio1_base + GPIO1_DATA_OUT_CLR) |= (GPIO_PIN_HIGH << (led_ctl_ptr->led_gpio_pin));
       led_ctl_ptr->is_led_on = false;
}

static void timer_handler(struct timer_list* timer_obj)
{
    LED_control_STC* led_ctl_ptr = (LED_control_STC*) timer_obj;

    if (led_ctl_ptr->led_curr_blink_times < led_ctl_ptr->led_max_blink_times)
    {
            if (true == led_ctl_ptr->is_led_on)
            {
                    switch_led_off(led_ctl_ptr);
            }
            else
            {
                    switch_led_on(led_ctl_ptr);
                    /* According to the requirement for the assignment
                     * the LED must be lit N times and non-lit states
                     * seem not to be taken into account */
                    ++(led_ctl_ptr->led_curr_blink_times);
            }

            mod_timer(&(led_ctl_ptr->led_timer), jiffies + led_ctl_ptr->led_blink_interval);
    }
    else
    {
            switch_led_off(led_ctl_ptr);
            printk(KERN_INFO "All done. The LED blinked %d times.\n", led_ctl_ptr->led_max_blink_times);
            if (0 != del_timer(&(led_ctl_ptr->led_timer)))
            {
                    printk(KERN_ERR "Cannot delete LED -associated timer.\n");
            }
    }
}

/* Module logic */

static int __init led_mod_init(void)
{    
       printk(KERN_INFO "LED interface loaded\n");

       gpio1_base = ioremap(GPIO1_MAP_BASE, DEFAULT_IOREMAP_LEN);
       if (NULL == gpio1_base)
       { 
               printk(KERN_ERR "Cannot remap GPIO1 area\n");
               return -EIO; 
       }

       *((reg_ptr_t) gpio1_base + GPIO1_OE_REG) &= ~(GPIO_OE_OUT << (led_usr3_ctl.led_gpio_pin));
       
       /* Init the timer */
       timer_setup(&led_usr3_ctl.led_timer, timer_handler, 0); 
       mod_timer(&led_usr3_ctl.led_timer, jiffies + led_usr3_ctl.led_blink_interval);
       
       return 0;
}
 
static void __exit led_mod_exit(void)
{ 
       /* Do cleanup */
       
       /* Switch off the LED */
       switch_led_off(&led_usr3_ctl);
       if (0 != del_timer(&led_usr3_ctl.led_timer))
       {
               printk(KERN_ERR "Cannot delete LED -associated timer.\n");
       }
       
       if (NULL != gpio1_base)
       {
               iounmap(gpio1_base);
       }
       
       printk(KERN_INFO "LED interface unloaded\n");
}
 
module_init(led_mod_init);
module_exit(led_mod_exit);
