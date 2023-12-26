PREFIX ?= /usr/bin

SRC += qrean.c
#SRC += pngle.c
SRC += miniz.c

CFLAGS += -I../src
LDFLAGS += ../src/libqrean.a -lm

all: qrean

qrean: $(SRC) ../src/libqrean.a
	cc -o qrean $(CFLAGS) $(SRC) $(LDFLAGS)

.PHONY: ../src/libqrean.a
../src/libqrean.a:
	make -C ../src

format:
	clang-format -i qrean.c

clean:
	rm -f qrean *.o
	make -C ../src clean

install: qrean
	install -m 0755 qrean $(PREFIX)
