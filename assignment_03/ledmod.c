#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Victor Krasnoshchok");
MODULE_DESCRIPTION("Blink a LED");
MODULE_VERSION("0.1");

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

/* Timer-related stuff */
static struct timer_list led_timer;
static unsigned long led_blink_times = 0;
static bool is_led_on = false;

/* **************************************** */

/* GPIO helpers */
static void switch_led_on(void)
{
       *((reg_ptr_t) gpio1_base + GPIO1_DATA_OUT_SET) |= (GPIO_PIN_HIGH << GPIO_LED_USR3_PIN);
       is_led_on = true;
}

static void switch_led_off(void)
{
       *((reg_ptr_t) gpio1_base + GPIO1_DATA_OUT_CLR) |= (GPIO_PIN_HIGH << GPIO_LED_USR3_PIN);
       is_led_on = false;
}

static void timer_handler(struct timer_list* timer_obj)
{
    (void) timer_obj; /* Not used */

    if (led_blink_times < MAX_LED_BLINK_TIMES)
    {
            if (true == is_led_on)
            {
                    switch_led_off();
            }
            else
            {
                    switch_led_on();
                    /* According to the requirement for the assignment
                     * the LED must be lit N times and non-lit states
                     * seem not to be taken into account */
                    ++led_blink_times;
            }

            mod_timer(&led_timer, jiffies + LED_LIGHT_INTERVAL_SEC);
    }
    else
    {
            printk(KERN_INFO "All done. The LED blinked %d times.\n", MAX_LED_BLINK_TIMES);
            del_timer(&led_timer);
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

       *((reg_ptr_t) gpio1_base + GPIO1_OE_REG) &= ~(GPIO_OE_OUT << GPIO_LED_USR3_PIN);
       
       /* Init the timer */
       timer_setup(&led_timer, timer_handler, 0); 
       mod_timer(&led_timer, jiffies + LED_LIGHT_INTERVAL_SEC);
       
       return 0;
}
 
static void __exit led_mod_exit(void)
{ 
       /* Do cleanup */
       
       /* Switch off the LED */
       switch_led_off();
       del_timer(&led_timer);
       
       if (NULL != gpio1_base)
       {
               iounmap(gpio1_base);
       }
       
       printk(KERN_INFO "LED interface unloaded\n");
}
 
module_init(led_mod_init);
module_exit(led_mod_exit);
