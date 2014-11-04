# WHEN YOU INSERT THE MODULE YOU NEED MOUSEDEV AND INPUT!

# Change it here or specify it on the "make" command line
KERNELVERSION = $(shell uname -r)
KERNELDIR = /usr/src/linux-${KERNELVERSION}

include $(KERNELDIR)/.config

#INCLUDE = -isystem $(KERNELDIR)/include
INCLUDE := -isystem /lib/modules/${KERNELVERSION}/build/include
WARN    := -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wno-unused-parameter
CFLAGS  := -O2 -DMODULE -D__KERNEL__ ${WARN} ${INCLUDE}
CC      := gcc

ifdef CONFIG_SMP
	CFLAGS += -D__SMP__ -DSMP
endif

all: ifeel.o ifeel_send

clean:
	rm -f *.o *~ core ifeel_send

device:
	mknod /dev/input/ifeel0 c 180 80
	chmod a+w /dev/input/ifeel0
	@printf "\n"
	@printf "WARNING!  /dev/input/ifeel0 is set to full write access!!\n"
	@printf "WARNING!  This could be a security risk!!\n"
	@printf "\n"

ins: ifeel.o
	insmod ifeel.o

rm: 
	rmmod ifeel

ifeel_send: ifeel_send.c ifeel.h
	${CC} -Wall -o ifeel_send ifeel_send.c
