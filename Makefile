obj-m += chardriver.o

chardriver-objs := driver.o field_element.o finite_field.o polynom.o binary_field_extension.o generator.o
PWD := $(CURDIR)

all:
	make -C /lib/modules/$(shell uname -r)/build   M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build   M=$(PWD) clean
