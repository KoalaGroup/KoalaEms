# Generated automatically from Makefile.in by configure.
# procs/lvd/qdc/Makefile.in
# $ZEL: Makefile.in,v 1.2 2006/08/04 19:10:32 wuestner Exp $
# created 05.Apr.2006 PW

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = qdc.c
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
