# Generated automatically from Makefile.in by configure.
#objects/var/Makefile.in
#$ZEL: Makefile.in,v 1.3 2002/11/06 15:37:11 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = variables.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../libobj.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#########################################################

variables.o: limits.stamp
