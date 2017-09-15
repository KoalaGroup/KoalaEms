# Generated automatically from Makefile.in by configure.
# trigger/vme/Makefile.in
# $ZEL: Makefile.in,v 1.4 2006/02/12 21:30:31 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = VMEpoll.c vmeirq.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

VMETYPES = $(shell echo $(unixvme)|tr '[:lower:]' '[:upper:]')
VMEFLAGS=$(foreach typ,$(VMETYPES),-DUNIXVME_$(typ) )
CFLAGS += $(VMEFLAGS)
export VMEFLAGS

lib = ../libtrig.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

#############################################

VMEpoll.r: trigger.stamp
