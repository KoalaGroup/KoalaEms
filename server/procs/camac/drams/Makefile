# Generated automatically from Makefile.in by configure.
# procs/camac/drams/Makefile.in
# $ZEL: Makefile.in,v 1.4 2002/11/06 15:37:17 wuestner Exp $

CONF = ../../../conf
srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make
CFLAGS += $(extracflags)

cfiles = drams.c c219_help.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

CAMACTYPES = $(shell echo $(camac)|tr '[:lower:]' '[:upper:]')
CAMACFLAGS=$(foreach typ,$(CAMACTYPES),-DCAMAC_$(typ) )
CFLAGS += $(CAMACFLAGS)

lib = ../../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#############################################

