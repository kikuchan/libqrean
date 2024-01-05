BUILDDIR?=`pwd`/../build/system/

all:
	BUILDDIR=$(BUILDDIR) make -C qrean
	BUILDDIR=$(BUILDDIR) make -C qrean-detect

clean:
	BUILDDIR=$(BUILDDIR) make -C qrean clean
	BUILDDIR=$(BUILDDIR) make -C qrean-detect clean

install:
	BUILDDIR=$(BUILDDIR) make -C qrean install
	BUILDDIR=$(BUILDDIR) make -C qrean-detect install
