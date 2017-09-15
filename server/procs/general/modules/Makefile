# Generated automatically from Makefile.in by configure.
# procs/general/modules/Makefile.in
# $ZEL: Makefile.in,v 1.1 2003/09/13 18:29:49 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = IdentIS.c ModuleParams.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

###################################################

