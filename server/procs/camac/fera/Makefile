# Generated automatically from Makefile.in by configure.
# procs/camac/fera/Makefile.in
# $ZEL: Makefile.in,v 1.6 2010/12/17 22:26:51 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = FERAmesspeds.c FERAreadout.c FERAsetup.c StandardFERA.c fsc_FERA.c
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

