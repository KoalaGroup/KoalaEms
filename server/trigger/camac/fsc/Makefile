# Generated automatically from Makefile.in by configure.
# trigger/camac/fsc/Makefile.in
# $ZEL: Makefile.in,v 1.3 2002/11/06 15:37:59 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = fsctrigger.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../../libtrig.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#############################################

