# Generated automatically from Makefile.in by configure.
# procs/general/crates/Makefile.in
#
# created 01.Jul.2009 PW
# $ZEL: Makefile.in,v 1.1 2009/07/03 18:35:13 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = crate_state.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#############################################################################

