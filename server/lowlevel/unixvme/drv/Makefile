# Generated automatically from Makefile.in by configure.
# lowlevel/unixvme/drv/Makefile.in
# $ZEL: Makefile.in,v 1.6 2007/09/17 00:35:51 wuestner Exp $

CONF = ../../../conf

srcdir = .


include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = vme_drv.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

CFLAGS += $(VMEFLAGS)

lib = ../../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend
