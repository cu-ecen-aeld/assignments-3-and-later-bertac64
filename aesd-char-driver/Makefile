# See example Makefile from scull project
# Comment/uncomment the following line to disable/enable debugging
#DEBUG = y

obj-m	:= aesdchar.o
aesdchar-objs := aesd-circular-buffer.o main.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
#	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions .*.o.d *.mod *.symvers *.order

