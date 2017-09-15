# Generated automatically from Makefile.in by configure.
# procs/camac/nofera/Makefile.in
# $ZEL: Makefile.in,v 1.1 2010/12/17 21:11:03 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = lc4300b.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

CAMACTYPES = $(shell echo $(camac)|tr '[:lower:]' '[:upper:]')
CAMACFLAGS=$(foreach typ,$(CAMACTYPES),-DCAMAC_$(typ) )
CFLAGS += $(CAMACFLAGS)

lib = ../../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#############################################

