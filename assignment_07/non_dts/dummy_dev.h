#ifndef __PLAT_DUMMY_DEVICE
#define __PLAT_DUMMY_DEVICE

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/mutex.h>
#include <asm/io.h>

#define DRV_NAME  "plat_dummy"

#define MEM_SIZE		(4096)
#define REG_SIZE		(4 * 4 * 4)
#define DEVICE_POLLING_TIME_MS	(500)

#define PLAT_IO_FLAGS_REG	(0) /* Offset of flags register */
#define PLAT_RD_SIZE_REG	(4) /* Offset of RD size */
#define PLAT_WR_SIZE_REG	(8) /* Offset of RW size */

/*
 * --------------------
 * 31.........| 1 | 0 | offset
 * --------------------
 * | reserved | w | r |
 * --------------------
 *  r = RD buffer ready
 *  w = WR buffer ready
 */

#define PLAT_RD_DATA_READY	(1) /* RD buffer ready - 000...01 */
#define PLAT_WR_DATA_READY	(2) /* WR buffer ready - 000...10 */

#define MAX_DUMMY_PLAT_THREADS	(2) /* Data processing threads */

struct plat_dummy_device {
	void __iomem		*rd_buf;
	void __iomem		*wr_buf;
	void __iomem		*regs;
	struct mutex            status_mtx;
	struct delayed_work	plat_rd_work;
	struct delayed_work	plat_wr_work;
	struct workqueue_struct *data_process_wq;
	u64 js_poll_time;
};

#endif
