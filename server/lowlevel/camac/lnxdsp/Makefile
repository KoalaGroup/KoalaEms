# Generated automatically from Makefile.in by configure.
# lowlevel/camac/lnxdsp/Makefile.in
# $ZEL: Makefile.in,v 1.5 2007/09/17 00:35:50 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = util.c lam.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

CFLAGS += $(CAMACFLAGS)

lib = ../../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#############################################

