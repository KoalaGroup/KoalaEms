# Generated automatically from Makefile.in by configure.
# procs/fastbus/test_sfi/Makefile.in
# $ZEL: Makefile.in,v 1.4 2002/11/06 15:37:33 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = fbtsingle.c fbloop.c fbbloop.c fbtblock.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

FASTBUSTYPES = $(shell echo $(fastbus)|tr '[:lower:]' '[:upper:]')
FASTBUSFLAGS=$(foreach typ,$(FASTBUSTYPES),-DFASTBUS_$(typ) )
CFLAGS += $(FASTBUSFLAGS)

lib = ../../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#######################################
