# Generated automatically from Makefile.in by configure.
# procs/unixvme/Makefile.in
# $ZEL: Makefile.in,v 1.5 2002/11/06 15:37:52 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = vme_primitiv.c vme_simple.c vme_verify.c vme_cratescan.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

UNIXVMETYPES = $(shell echo $(unixvme)|tr '[:lower:]' '[:upper:]')
UNIXVMEFLAGS=$(foreach typ,$(UNIXVMETYPES),-DUNIXVME_$(typ) )
CFLAGS += $(UNIXVMEFLAGS)

lib = ../libprocs.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

clean:
	-$(RM) *.o

depend: xdepend

.PHONY: depend

#############################################
