# Generated automatically from Makefile.in by configure.
# trigger/camac/drams/Makefile.in
# $ZEL: Makefile.in,v 1.4 2007/09/17 00:35:52 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make
lib = ../../libtrig.a

cfiles = dramstrigger.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#############################################
