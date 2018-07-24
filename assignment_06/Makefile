ifeq ($(KERNELRELEASE),)

KERNELDIR ?= $(BBB_KERNEL_SRC)

module:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) C=1 modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) C=1 clean

.PHONY: module clean

else

CFLAGS_hello.o := -DDEBUG
obj-m := synctest.o

endif
