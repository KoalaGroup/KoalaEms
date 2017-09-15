# Generated automatically from Makefile.in by configure.
# procs/general/vars/Makefile.in
# $ZEL: Makefile.in,v 1.4 2002/11/06 15:37:37 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = GetVar.c IncIntVar.c SetIntVar.c histo.c GetVarRange.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#############################################################################

