obj-m += chardriver.o

chardriver-objs := driver.o
PWD := $(CURDIR)

all:
	make -C ~/vm/kernel  M=$(PWD) modules
clean:
	make -C ~/vm/kernel  M=$(PWD) clean
