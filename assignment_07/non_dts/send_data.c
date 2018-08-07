#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define OUT_BUF_BASE	0x9f200000
#define IN_BUF_BASE	0x9f201000

#define REG_BASE	0x9f202000

#define MEM_SIZE	(1024)
#define REG_SIZE	(12)

#define OUT_BUF_DATA_READY	(1)
#define IN_BUF_DATA_READY	(2) 

extern int errno;

int main(int argc, char **argv)
{
	volatile unsigned int *reg_addr = NULL, *count_addr, *flag_addr;
	volatile unsigned char *mem_out_addr = NULL;
	volatile unsigned char *mem_in_addr = NULL;
	unsigned int i,j, num = 0, val;
	char drv_data;

	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd < 0)
	{
		printf("Can't open /dev/mem\n");
		return -1;
	}

	mem_out_addr = (unsigned char *) mmap(0, MEM_SIZE, PROT_WRITE, MAP_SHARED, fd, OUT_BUF_BASE);
	
	if(mem_out_addr == NULL)
	{
		printf("Can't mmap OUT buffer\n");
		return -1;
	}

	mem_in_addr = (unsigned char *) mmap(0, MEM_SIZE, PROT_READ, MAP_SHARED, fd, IN_BUF_BASE);

	if(mem_in_addr == NULL)
	{
		printf("Can't mmap IN buffer\n");
		return -1;
	}
	
	reg_addr = (unsigned int *) mmap(0, REG_SIZE, PROT_WRITE |PROT_READ, MAP_SHARED, fd, REG_BASE);


	/* ---------------------------------- */
	/* Write to our dummy device          */


	flag_addr = reg_addr;
	count_addr = reg_addr;
	count_addr++;
	
	for (i=0; i < 50; i++) {
		*mem_out_addr++ = 0x41 + i;
	}
	
	*count_addr = 50;
	*flag_addr = OUT_BUF_DATA_READY;

	/* ---------------------------------- */
	/* Now read from our dummy device     */

	count_addr++;
	for (i = 0; i < 50; i++) {
		do {
			/* Wait for transfer in polling mode */
			/* 
			 * It would be good to have "sleep" here
			 * but I was too lazy and sleepy. Sorry.
			 */
			val = *flag_addr;
		} while (val & IN_BUF_DATA_READY == 0);

		printf("----- DATA count to receive: %d\n", *count_addr);

		for (j = 0; j < *count_addr; j++) {
			drv_data = *((char *)(mem_in_addr + j));
			printf("0x%x - %c\n", drv_data, drv_data);
		}

		*flag_addr = 0; /* Clear all transfer statuses */
	}

	return 0;
}
