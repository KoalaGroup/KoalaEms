# Generated automatically from Makefile.in by configure.
# dataout/ringbuffers/Makefile.in
# $ZEL: Makefile.in,v 1.4 2007/09/17 00:35:50 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = interface.c ringbuffers.c handler.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../libdo.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#############################################

