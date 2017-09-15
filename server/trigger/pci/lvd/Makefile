# Generated automatically from Makefile.in by configure.
# trigger/pci/lvd/Makefile.in
# $ZEL: Makefile.in,v 1.5 2006/02/12 21:35:05 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

LVDTYPES = $(shell echo $(lvd)|tr '[:lower:]' '[:upper:]')
LVDFLAGS=$(foreach typ,$(LVDTYPES),-DLVD_$(typ) )
CFLAGS += $(LVDFLAGS)
export LVDFLAGS

lib = ../../libtrig.a

cfiles = lvdtrigger.c lvdtrigger_async.c #LVDpoll.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#############################################
