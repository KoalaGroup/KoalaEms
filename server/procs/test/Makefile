# Generated automatically from Makefile.in by configure.
# procs/test/Makefile.in
# $ZEL: Makefile.in,v 1.9 2005/04/12 18:40:15 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = debuglevel.c delay.c error.c exit.c random.c unsol.c \
  onreadouterror.c chstdout.c maxevcount.c readoutprior.c printevent.c \
  select_statist.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#####################################################

