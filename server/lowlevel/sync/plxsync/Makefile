# Generated automatically from Makefile.in by configure.
# lowlevel/plxsync/Makefile.in
# $ZEL: Makefile.in,v 1.3 2007/09/17 00:35:51 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = plxsync.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

CFLAGS += $(SYNCFLAGS)

lib = ../../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

######################################################

