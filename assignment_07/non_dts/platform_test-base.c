#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <asm/io.h>

#include "dummy_dev.h"
#include "platform_test-utils.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Victor Krasnoshchok <militant.daos@gmail.com>");
MODULE_DESCRIPTION("Dummy platform driver");
MODULE_VERSION("0.1");

/* 
*  The device has 3 resources:
*  1) 4K of memory at address 0x9f200000 - read buffer to
*     receive data from userspace;
*  2) 4K of memory at address 0x9f201000 - write buffer
*     for sending data to userspace;
*  3) Three 32-bit registers at address 0x9f202000;
*  2.1. Flag Register: 0x9f202000
*	bit 0: PLAT_RD_DATA_READY - is set to 1 if data from userspace is ready.
*	bit 1: PLAT_WR_DATA_READY - is set to 1 if data to userspace is ready.
*	other bits: reserved;
*  2.2. RD data size register - data size received from userspace (0..4095);
*  2.3. WR data size register - data size to be sent to userspace (0..4095);
*/

static const dma_addr_t rd_buf_base	= 0x9f200000;
static const dma_addr_t wr_buf_base	= 0x9f201000;
static const dma_addr_t reg_base	= 0x9f202000;

static struct platform_device *pdev;

static const char dummy_usr_msg[] = ">> Dummy message << ";
static const u32 dummy_usr_msg_full = sizeof(dummy_usr_msg) +
					sizeof(u32);

static void plat_dummy_rd_work(struct work_struct *work)
{
	struct plat_dummy_device *my_device;
	u32 i, size;
	u8 data;

	pr_info("++%s(%u)\n", __func__, jiffies_to_msecs(jiffies));

	my_device = container_of(work, struct plat_dummy_device, plat_rd_work.work);

	if (plat_dummy_is_rd_buf_ready(my_device, &size)) {

		pr_info("%s: size to read = %d\n", __func__, size);

		if (size > MEM_SIZE)
			size = MEM_SIZE;

		for(i = 0; i < size; i++) {
			data = plat_dummy_read_byte(my_device, i);
			pr_info("%s: mem[%d] = 0x%x ('%c')\n", __func__,  
				i, data, data);
		}

		rmb();

		/* Reset data ready flag to signalize
		 * the userspace app. that the device
		 * has completed reading from the
		 * input buffer.
		 */
		plat_dummy_clear_rd_buf_ready(my_device);
	}

	queue_delayed_work(my_device->data_process_wq, &my_device->plat_rd_work,
			my_device->js_poll_time);
}

static void plat_dummy_wr_work(struct work_struct *work)
{
	struct plat_dummy_device *my_device;
	u32 i, size;
	u32 curr_jiffies;
	u32 j = sizeof(u32) - 1; /* For jiffies value serialization */

	pr_info("++%s(%u)\n", __func__, jiffies_to_msecs(jiffies));

	my_device = container_of(work, struct plat_dummy_device, 
				plat_wr_work.work);

	if (!plat_dummy_is_wr_buf_ready(my_device)) {

		pr_info("Data transfer started.");

		size = dummy_usr_msg_full;
		if (size > MEM_SIZE)
			size = MEM_SIZE;

		/* Write "dummy" part of the message ... */
		for (i = 0; i < (size - sizeof(u32)); i++) {
			plat_dummy_write_byte(my_device, i, dummy_usr_msg[i]);
		}

		curr_jiffies = jiffies_to_msecs(jiffies);

		/* ... and append four bytes of jiffies to the end. */
		for (; i < size; i++) {
			plat_dummy_write_byte(my_device, i, 
					((u8 *) &curr_jiffies)[j]);
			j--;
		}


		wmb();

		/*
		 * Signalize the userspace app that the
		 * data is ready to be transferred.
		 */

		plat_dummy_set_wr_buf_ready(my_device, (size - 1));
	}

	queue_delayed_work(my_device->data_process_wq, &my_device->plat_wr_work,
			my_device->js_poll_time);
}

static int plat_dummy_probe(struct platform_device *pdev)
{
	struct	device *dev = &pdev->dev;
	struct	plat_dummy_device *my_device;
	struct	resource *res;

	pr_info("++%s\n", __func__);

	my_device = devm_kzalloc(dev, sizeof(struct plat_dummy_device), 
			GFP_KERNEL);
	if (!my_device)
		return -ENOMEM;

	/*
	 * Get RD buffer resource & ioremap it into kernel's address
	 * space.
	 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pr_info("res 0 (RD buffer) = %zx..%zx\n", res->start, res->end);

	my_device->rd_buf = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(my_device->rd_buf))
		return PTR_ERR(my_device->rd_buf);
	/* --------------------------------------------------------- */

	/*
	 * Get WR buffer resource & ioremap it into kernel's address
	 * space.
	 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	pr_info("res 1 (WR buffer) = %zx..%zx\n", res->start, res->end);

	my_device->wr_buf = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(my_device->wr_buf))
		return PTR_ERR(my_device->wr_buf);

	/* --------------------------------------------------------- */

	/*
	 * Get regs resource & ioremap it into kernel's address
	 * space.
	 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	pr_info("res 2 (regs) = %zx..%zx\n", res->start, res->end);

	my_device->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(my_device->regs))
		return PTR_ERR(my_device->regs);

	/* --------------------------------------------------------- */

	platform_set_drvdata(pdev, my_device);

	pr_info("RD buffer (usr->krn) mapped to %p\n", my_device->rd_buf);
	pr_info("WR buffer (krn->usr) mapped to %p\n", my_device->wr_buf);
	pr_info("Registers mapped to %p\n", my_device->regs);

	/*Init data processing WQ*/
	my_device->data_process_wq = alloc_workqueue("plat_dummy_workqueue",
					WQ_UNBOUND, MAX_DUMMY_PLAT_THREADS);

	if (!my_device->data_process_wq)
		return -ENOMEM;

	mutex_init(&my_device->status_mtx);

	INIT_DELAYED_WORK(&my_device->plat_rd_work, plat_dummy_rd_work);
	INIT_DELAYED_WORK(&my_device->plat_wr_work, plat_dummy_wr_work);

	my_device->js_poll_time = msecs_to_jiffies(DEVICE_POLLING_TIME_MS);
	queue_delayed_work(my_device->data_process_wq, 
			&my_device->plat_rd_work, 0);
	queue_delayed_work(my_device->data_process_wq, 
			&my_device->plat_wr_work, 0);

	/*
	 * It seems there is no need to check ERR_PTR_OR_ZERO
	 * here because all necessary checks are already
	 * performed above.
	 */

	return 0;
}

static int plat_dummy_remove(struct platform_device *pdev)
{
	struct plat_dummy_device *my_device = platform_get_drvdata(pdev);

	pr_info("++%s\n", __func__);

	if (my_device->data_process_wq) {

		/* Destroy the workqueue */

		cancel_delayed_work_sync(&my_device->plat_rd_work);
		cancel_delayed_work_sync(&my_device->plat_wr_work);
		destroy_workqueue(my_device->data_process_wq);
	}

        return 0;
}

static int __init plat_dummy_device_add(void)
{
	int err;

	struct resource res[3] = {{
		.start	= rd_buf_base,
		.end	= rd_buf_base + MEM_SIZE - 1,
		.name	= "dummy_rd_buf",
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= wr_buf_base,
		.end	= wr_buf_base + MEM_SIZE - 1,
		.name	= "dummy_wr_buf",
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= reg_base,
		.end	= reg_base + REG_SIZE - 1,
		.name	= "dummy_regs",
		.flags	= IORESOURCE_MEM,
	}};

	pr_info("++%s\n", __func__);

	pdev = platform_device_alloc(DRV_NAME, res[0].start);
	if (!pdev) {
		err = -ENOMEM;
		pr_err("Device allocation failed\n");
		goto exit;
	}

	err = platform_device_add_resources(pdev, res, ARRAY_SIZE(res));
	if (err) {
		pr_err("Device resource addition failed (%d)\n", err);
		goto exit_device_put;
	}

	err = platform_device_add(pdev);
	if (err) {
		pr_err("Device addition failed (%d)\n", err);
		goto exit_device_put;
	}

	return 0;

 exit_device_put:
	platform_device_put(pdev);
 exit:
	pdev = NULL;
	return err;
}

static struct platform_driver plat_dummy_driver = {
	.driver = {
		.name	= DRV_NAME,
	},
	.probe		= plat_dummy_probe,
	.remove		= plat_dummy_remove,
};

static int plat_dummy_driver_register(void)
{
	int res;


	res = platform_driver_register(&plat_dummy_driver);
	if (res)
		goto exit;

	res = plat_dummy_device_add();
	if (res)
		goto exit_unreg_driver;

	return 0;

 exit_unreg_driver:
	platform_driver_unregister(&plat_dummy_driver);
 exit:
	return res;
}

static void plat_dummy_unregister(void)
{
	if (pdev) {
		platform_device_unregister(pdev);
		platform_driver_unregister(&plat_dummy_driver);
	}
}


static int __init plat_dummy_init_module(void)
{
	pr_info("Platform dummy test module init\n");
	return plat_dummy_driver_register();
}

static void __exit plat_dummy_cleanup_module(void)
{
	pr_info("Platform dummy test module exit\n");
	plat_dummy_unregister();
	return;
}

module_init(plat_dummy_init_module);
module_exit(plat_dummy_cleanup_module);
