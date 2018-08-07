#include "dummy_dev.h"
#include "platform_test-utils.h"

static u32 plat_dummy_reg_read32(struct plat_dummy_device *my_dev, u32 offset)
{
	return ioread32(my_dev->regs + offset);
}

static void plat_dummy_reg_write32(struct plat_dummy_device *my_dev,
					u32 offset, u32 val)
{
	iowrite32(val, my_dev->regs + offset);
}

/* ------------------------------------------------------------------ */

u8 plat_dummy_read_byte(struct plat_dummy_device *my_dev, u32 offset)
{
	return ioread8(my_dev->rd_buf + offset);
}

void plat_dummy_write_byte(struct plat_dummy_device *my_dev, u32 offset, u8 data)
{
	iowrite8(data, my_dev->wr_buf + offset);
}

bool plat_dummy_is_rd_buf_ready(struct plat_dummy_device *my_dev, u32 *data_size)
{
	u32 status_reg;

	mutex_lock(&my_dev->status_mtx);
	status_reg = plat_dummy_reg_read32(my_dev, PLAT_IO_FLAGS_REG);
	mutex_unlock(&my_dev->status_mtx);

	if (status_reg & PLAT_RD_DATA_READY) {
		*data_size = plat_dummy_reg_read32(my_dev, PLAT_RD_SIZE_REG);
		return true;
	}

	return false;
}

void plat_dummy_clear_rd_buf_ready(struct plat_dummy_device *my_dev)
{
	u32 status_reg;

	mutex_lock(&my_dev->status_mtx);
	status_reg = plat_dummy_reg_read32(my_dev, PLAT_IO_FLAGS_REG);
	status_reg &= ~PLAT_RD_DATA_READY;
	plat_dummy_reg_write32(my_dev, PLAT_IO_FLAGS_REG, status_reg);
	mutex_unlock(&my_dev->status_mtx);
}

bool plat_dummy_is_wr_buf_ready(struct plat_dummy_device *my_dev)
{
	u32 status_reg;

	mutex_lock(&my_dev->status_mtx);
	status_reg = plat_dummy_reg_read32(my_dev, PLAT_IO_FLAGS_REG);
	mutex_unlock(&my_dev->status_mtx);

	if (status_reg & PLAT_WR_SIZE_REG)
		return true;

	return false;
}

void plat_dummy_set_wr_buf_ready(struct plat_dummy_device *my_dev, u32 data_size)
{
	u32 status_reg;

	plat_dummy_reg_write32(my_dev, PLAT_WR_SIZE_REG, data_size);

	wmb();

	mutex_lock(&my_dev->status_mtx);
	status_reg = plat_dummy_reg_read32(my_dev, PLAT_IO_FLAGS_REG);
	status_reg |= PLAT_WR_DATA_READY;
	plat_dummy_reg_write32(my_dev, PLAT_IO_FLAGS_REG, status_reg);
	mutex_unlock(&my_dev->status_mtx);
}
