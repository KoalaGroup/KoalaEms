# Generated automatically from Makefile.in by configure.
# procs/hv/sy527/Makefile.in
# $ZEL: Makefile.in,v 1.1 2007/08/07 11:13:21 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = sy527.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

clean:
	-$(RM) *.o

depend: xdepend

.PHONY: depend

#############################################
