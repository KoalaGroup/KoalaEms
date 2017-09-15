# Generated automatically from Makefile.in by configure.
# procs/caenet/Makefile.in
# $ZEL: Makefile.in,v 1.1 2007/03/20 20:39:36 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = caenet.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

CAENTYPES = $(shell echo $(caenet)|tr '[:lower:]' '[:upper:]')
CAENFLAGS=$(foreach typ,$(CAENTYPES),-DCAEN_$(typ) )
CFLAGS += $(CAENFLAGS)

lib = ../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

clean:
	-$(RM) *.o

depend: xdepend

.PHONY: depend

#############################################

