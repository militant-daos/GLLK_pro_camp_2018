#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Victor Krasnoshchok");
MODULE_DESCRIPTION("Compute sum of two args");
MODULE_VERSION("0.1");

int first_arg  = 0;
int second_arg = 0;

module_param(first_arg, int, S_IRUGO); //just can be read
MODULE_PARM_DESC(first_arg, "The the first int arg of the sum");  ///< parameter description
 
module_param(second_arg, int, S_IRUGO); //just can be read
MODULE_PARM_DESC(first_arg, "The the second int arg of the sum");  ///< parameter description

static int __init hello_init(void)
{
        printk(KERN_INFO "The sum is: %d\n", (first_arg + second_arg));
        return 0;
}
 
static void __exit hello_exit(void)
{
        printk(KERN_INFO "D'oh!\n");
}
 
module_init(hello_init);
module_exit(hello_exit);

