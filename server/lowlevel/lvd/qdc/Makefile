# Generated automatically from Makefile.in by configure.
# lowlevel/lvd/qdc/Makefile.in
# $ZEL: Makefile.in,v 1.2 2006/08/04 19:10:16 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = lvd_qdc.c qdc.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

LVDTYPES = $(shell echo $(lvd)|tr '[:lower:]' '[:upper:]')
LVDFLAGS=$(foreach typ,$(LVDTYPES),-DLVD_$(typ) )
CFLAGS += $(LVDFLAGS)
export LVDFLAGS

lib = ../../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend
