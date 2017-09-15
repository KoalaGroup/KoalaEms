# Generated automatically from Makefile.in by configure.
# procs/wincc/Makefile.in
# $ZEL: Makefile.in,v 1.3 2002/11/06 15:37:55 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make

include $(CONF)/config.make
CFLAGS += $(extracflags)

cfiles = wincc_rw.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

###################################################

