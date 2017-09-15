# Generated automatically from Makefile.in by configure.
# procs/lvd/tdc/Makefile.in
# $ZEL: Makefile.in,v 1.2 2007/03/02 22:46:30 wuestner Exp $
# created 06.Sep.2002 PW

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = tdc_init.c f1_init.c gpx_init.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

LVDTYPES = $(shell echo $(lvd)|tr '[:lower:]' '[:upper:]')
LVDFLAGS=$(foreach typ,$(LVDTYPES),-DLVD_$(typ) )
CFLAGS += $(LVDFLAGS)

lib = ../../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

clean:
	-$(RM) *.o

depend: xdepend

.PHONY: depend

#############################################
