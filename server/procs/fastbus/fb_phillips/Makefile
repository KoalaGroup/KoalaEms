# Generated automatically from Makefile.in by configure.
# procs/fastbus/fb_phillips/Makefile.in
# $ZEL: Makefile.in,v 1.4 2002/11/06 15:37:28 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = fb_ph10cx_init.c fb_ph10cx_read.c

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
