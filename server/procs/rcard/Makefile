# Generated automatically from Makefile.in by configure.
# procs/rcard/Makefile.in
# $ZEL: Makefile.in,v 1.7 2007/09/17 00:35:52 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = RCardProcs.c \
	 CAENV550readout.c \
	 CAENV775readout.c \
	 SIS3801readout.c \
	 VPGwriteVA.c \
	 VPGbitTest.c
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







