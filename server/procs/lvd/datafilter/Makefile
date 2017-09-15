# Generated automatically from Makefile.in by configure.
# procs/lvd/datafilter/Makefile.in
# $ZEL: Makefile.in,v 1.1 2010/02/05 14:38:11 wuestner Exp $
# created 2010-02-04 PW

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = datafilter.c
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
