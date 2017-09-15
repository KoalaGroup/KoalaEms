# Generated automatically from Makefile.in by configure.
# lowlevel/cosybeam/Makefile.in
# $ZEL: Makefile.in,v 1.3 2007/09/17 00:35:50 wuestner Exp $

CONF = ../../conf

srcdir = .


include $(CONF)/rules.make
include $(CONF)/config.make

CFLAGS := $(CFLAGS)
cfiles = cosybeam.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

######################################################
