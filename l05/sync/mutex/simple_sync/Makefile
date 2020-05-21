obj-m := simple_module.o

KDIR  := $(HOME)/agh/linux419
PWD   := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

