# Generated automatically from Makefile.in by configure.
# lowlevel/can/cpci_can331/Makefile.in
# $ZEL: Makefile.in,v 1.2 2006/04/20 09:43:58 peterka Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make

cfiles = cpci_can331.c libpcan.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

CFLAGS += $(CANFLAGS)

lib = ../../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

######################################################

