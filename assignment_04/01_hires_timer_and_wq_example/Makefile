obj-m += hrt.o

KDIR := $(BBB_KERNEL_SRC) 

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
