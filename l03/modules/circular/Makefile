obj-m := kmltml_circular.o

KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

ccflags-y := -Wno-declaration-after-statement

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

