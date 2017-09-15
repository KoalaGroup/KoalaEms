# Generated automatically from Makefile.in by configure.
# procs/general/modules/Makefile.in
# $ZEL: Makefile.in,v 1.1 2007/02/11 22:29:20 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = proc_eventparse.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

###################################################

