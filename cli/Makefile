BUILDDIR?=$(abspath ../build/system)

all:
	@BUILDDIR=$(BUILDDIR) $(MAKE) -C qrean
	@BUILDDIR=$(BUILDDIR) $(MAKE) -C qrean-detect

clean:
	@BUILDDIR=$(BUILDDIR) $(MAKE) -C qrean clean
	@BUILDDIR=$(BUILDDIR) $(MAKE) -C qrean-detect clean

install:
	@BUILDDIR=$(BUILDDIR) $(MAKE) -C qrean install
	@BUILDDIR=$(BUILDDIR) $(MAKE) -C qrean-detect install
