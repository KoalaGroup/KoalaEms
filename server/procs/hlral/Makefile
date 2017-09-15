# Generated automatically from Makefile.in by configure.
# procs/hlral/Makefile.in
# $ZEL: Makefile.in,v 1.5 2007/09/17 00:35:51 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = hlralops.c hlraltest.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

clean:
	-$(RM) *.o

depend: xdepend

.PHONY: depend

#############################################

