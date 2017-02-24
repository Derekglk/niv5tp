
obj-m	:= custom_module.o
PWD       := $(shell pwd)
KERNELDIR=/home/orcad/RESOURCES/UVF12B501/resources/linux-xlnx
CC=arm-xilinx-linux-gnueabi-gcc

default:app
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf *.o *~ core .*.cmd *.ko *.mod.c .tmp_versions Module.symvers modules.order app


install:default app
	sudo mkdir -p /srv/nfsroot/root/TP3
	sudo cp -r *.sh *.ko app /srv/nfsroot/root/TP3

app:app.c
	$(CC) app.c -o app
