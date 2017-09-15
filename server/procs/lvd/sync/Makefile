# Generated automatically from Makefile.in by configure.
# procs/lvd/sync/Makefile.in
# $ZEL: Makefile.in,v 1.1 2007/02/11 22:28:17 wuestner Exp $
# created 2007-01-10 PW

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = sync_init.c sync_statist.c
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
