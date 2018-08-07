KERNELDIR ?= $(BBB_KERNEL_SRC)

obj-m := platform_test.o 
platform_test-objs := platform_test-utils.o platform_test-base.o

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) C=1 modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) C=1 clean

