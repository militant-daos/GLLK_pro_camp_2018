#ifndef __DUMMY_DEV_UTILS_H
#define __DUMMY_DEV_UTILS_H

/*
 * Buffers data access routines.
 */
u8 plat_dummy_read_byte(struct plat_dummy_device *my_dev, u32 offset);
void plat_dummy_write_byte(struct plat_dummy_device *my_dev, u32 offset, u8 data);

/*
 * RD (input) buffer status manipulation.
 */
bool plat_dummy_is_rd_buf_ready(struct plat_dummy_device *my_dev, u32 *data_size);
void plat_dummy_clear_rd_buf_ready(struct plat_dummy_device *my_dev);

/*
 * WR (output) buffer status manipulation.
 */
bool plat_dummy_is_wr_buf_ready(struct plat_dummy_device *my_dev);
void plat_dummy_set_wr_buf_ready(struct plat_dummy_device *my_dev, u32 data_size);

#endif
