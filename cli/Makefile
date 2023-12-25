PREFIX ?= /usr/bin

SRC += main.c
#SRC += pngle.c
SRC += miniz.c

CFLAGS += -I../src
LDFLAGS += ../src/libqrean.a -lm

all: qrean

qrean: $(SRC) ../src/libqrean.a
	cc -o qrean $(CFLAGS) $(SRC) $(LDFLAGS)

../src/libqrean.a:
	make -C ../src

format:
	clang-format -i main.c

clean:
	rm -f qrean *.o

install: qrean
	install -m 0755 qrean $(PREFIX)
