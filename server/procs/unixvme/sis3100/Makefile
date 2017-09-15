# Generated automatically from Makefile.in by configure.
# procs/unixvme/sis3100/Makefile.in
# $ZEL: Makefile.in,v 1.2 2006/02/12 21:45:09 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = sis3100dsp.c vme_dspprocs.c sis3100rw.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

UNIXVMETYPES = $(shell echo $(unixvme)|tr '[:lower:]' '[:upper:]')
UNIXVMEFLAGS=$(foreach typ,$(UNIXVMETYPES),-DUNIXVME_$(typ) )
CFLAGS += $(UNIXVMEFLAGS)

lib = ../../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

clean:
	-$(RM) *.o

depend: xdepend

.PHONY: depend

#############################################
