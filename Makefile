.PHONY: all clean examples

all: src
	make -C src

clean:
	make -C src clean

examples:
	make -C examples

install-examples:
	make -C examples install

clean-examples:
	make -C examples clean
