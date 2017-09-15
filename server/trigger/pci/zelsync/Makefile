# Generated automatically from Makefile.in by configure.
# trigger/pci/zelsync/Makefile.in
# $ZEL: Makefile.in,v 1.5 2002/11/06 15:38:03 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = zelsynctrigger.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../../libtrig.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#############################################

