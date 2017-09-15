# Generated automatically from Makefile.in by configure.
#objects/pi/readout_em_cluster/Makefile.in
#$ZEL: Makefile.in,v 1.4 2007/09/17 00:35:51 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = datains.c piops.c di_cluster.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = ../../libobj.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(AR) cr $@ $(ofiles)
	$(RANLIB) $@

depend: xdepend

.PHONY: depend
