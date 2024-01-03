PREFIX ?= /usr/bin
QREAN?=../../src

MAIN=qrean-detect

SRC += $(MAIN).c
SRC += pngle.c
SRC += miniz.c

CFLAGS += -I$(QREAN)
LDFLAGS += $(QREAN)/libqrean.a -lm

all: $(MAIN)

$(MAIN): $(SRC) $(QREAN)/libqrean.a
	$(CC) -o $(MAIN) $(CFLAGS) $(SRC) $(LDFLAGS)

$(QREAN)/libqrean.a:
	make -C $(QREAN)

format:
	clang-format -i $(MAIN).c

clean:
	rm -f $(MAIN) $(MAIN).exe *.o
	make -C $(QREAN) clean

install: $(MAIN)
	install -m 0755 $(MAIN) $(PREFIX)
