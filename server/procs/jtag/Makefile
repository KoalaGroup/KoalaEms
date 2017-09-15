# Generated automatically from Makefile.in by configure.
# procs/jtag/Makefile.in
# $ZEL: Makefile.in,v 1.1 2006/08/04 19:26:56 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = procjtag.c
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
