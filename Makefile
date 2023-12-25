.PHONY: all clean examples cli

all: src cli

src: 
	@make -C src

cli: src
	@make -C cli

install: cli
	@make -C cli install

clean:
	@make -C src clean
	@make -C cli clean
