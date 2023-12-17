obj-m = hw2.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

run: default
	sudo rmmod hw2
	sudo insmod hw2.ko
	# cat /proc/hw2

clean:
	rm -rf *.ko *.mod *.mod.* .*.cmd *.o *.symvers *.order