# Generated automatically from Makefile.in by configure.
# trigger/vme/sis3100/Makefile.in
# $ZEL: Makefile.in,v 1.2 2007/03/24 21:34:46 wuestner Exp $
# created 2006-Jul-09 PW

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = sis3100trigger.c sis3100_zelsync.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../../libtrig.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#############################################

