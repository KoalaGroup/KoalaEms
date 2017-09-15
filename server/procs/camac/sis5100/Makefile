# Generated automatically from Makefile.in by configure.
# procs/camac/sis5100/Makefile.in
# $ZEL: Makefile.in,v 1.1 2006/08/04 19:25:04 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = sis5100raw.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

CAMACTYPES = $(shell echo $(camac)|tr '[:lower:]' '[:upper:]')
CAMACFLAGS=$(foreach typ,$(CAMACTYPES),-DCAMAC_$(typ) )
CFLAGS += $(CAMACFLAGS)

lib = ../../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

clean:
	-$(RM) *.o

depend: xdepend

.PHONY: depend

#############################################
