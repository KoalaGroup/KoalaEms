# Generated automatically from Makefile.in by configure.
# objects/domain/Makefile.in
# $ZEL: Makefile.in,v 1.4 2002/11/06 15:37:02 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make

include $(CONF)/config.make

ofiles = domain.o $(foreach d,$(domains),dom_$(d).o)
depfiles = domain.c $(foreach d,$(domains),dom_$(d).c)

lib = ../libobj.a

all: $(lib)

$(lib): $(lib)($(ofiles)) domains.stamp
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

###################################################
