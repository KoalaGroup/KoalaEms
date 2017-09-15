# Generated automatically from Makefile.in by configure.
# procs/pci/trigger/Makefile.in
# $ZEL: Makefile.in,v 1.7 2002/11/06 15:37:44 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

CPPFLAGS := $(CPPFLAGS) 
cfiles = pcitrigger.c pcitrigdata.c force_dt.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

###################################################
