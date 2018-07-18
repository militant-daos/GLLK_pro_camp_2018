#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define GPIO2_8 (32 + 32 + 8)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Victor Krasnoshchok");
MODULE_DESCRIPTION("IRQ example");
MODULE_VERSION("0.1");

/* ************************************************* */

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

static void __iomem* gpio1_base;
static bool is_led_on = false;

/* ************************************************* */

static struct tasklet_struct tasklet_irq;
static int irq_num = -1;
static struct device usr_btn = { 0 };

/* ************* USR LED helpers ******************* */

/* GPIO helpers */
static void toggle_led(void)
{
    if (false == is_led_on)
    {
        *((reg_ptr_t) gpio1_base + GPIO1_DATA_OUT_SET) |= (GPIO_PIN_HIGH << GPIO_LED_USR3_PIN);
    }
    else
    {
        *((reg_ptr_t) gpio1_base + GPIO1_DATA_OUT_CLR) |= (GPIO_PIN_HIGH << GPIO_LED_USR3_PIN);
    }
    is_led_on = !is_led_on;
}

/* ***************** IRQ Stuff ******************** */

static void irq_tl_callback(unsigned long data)
{
    printk(KERN_INFO "*\n");
    toggle_led();
}

static irqreturn_t irq_test_handler(int irq, void* dev_id)
{
    (void) irq;
    (void) dev_id;

    tasklet_schedule(&tasklet_irq);
    return IRQ_HANDLED;
}

static int __init test_irq_init(void)
{
    printk(KERN_INFO "IRQ test module loaded.\n");
    
    /* Auxiliary stuff: init GPIO bound to USR LED3 */
    gpio1_base = ioremap(GPIO1_MAP_BASE, DEFAULT_IOREMAP_LEN);
    if (NULL == gpio1_base)
    { 
        printk(KERN_ERR "Cannot remap GPIO1 area\n");
	    return -EIO; 
    }

    *((reg_ptr_t) gpio1_base + GPIO1_OE_REG) &= ~(GPIO_OE_OUT << GPIO_LED_USR3_PIN);

    /* Tasklet & IRQ setup */

    tasklet_init(&tasklet_irq, irq_tl_callback, 0);
    
    if (0 != gpio_request(GPIO2_8, "user_button_pins"))
    {
        printk(KERN_INFO "Failed to request GPIO.\n");
        return -EIO;
    }

    gpio_direction_input(GPIO2_8);
    irq_num = gpio_to_irq(GPIO2_8);

    printk(KERN_INFO "IRQ num: %d\n", irq_num);

    if (0 != request_irq(irq_num, irq_test_handler, IRQF_SHARED | IRQF_TRIGGER_RISING, "USR_button", (void*) &usr_btn))
    {
        printk(KERN_ERR "Failed to request IRQ.\n");
        return -EBUSY;
    }

    return 0;
}

static void __exit test_irq_exit(void)
{
    if (-1 != irq_num)
    {
        free_irq(irq_num, (void*) &usr_btn);
        gpio_free(GPIO2_8);
    }

    tasklet_kill(&tasklet_irq);

    if (NULL != gpio1_base)
    {
        iounmap(gpio1_base);
    }

    printk(KERN_INFO "IRQ test module unloaded.\n");
}

module_init(test_irq_init);
module_exit(test_irq_exit);
