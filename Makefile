obj-m += chardriver.o

chardriver-objs := driver.o field_element.o finite_field.o polynom.o binary_field_extension.o
PWD := $(CURDIR)

all:
	make -C ~/vm/kernel  M=$(PWD) modules
clean:
	make -C ~/vm/kernel  M=$(PWD) clean
