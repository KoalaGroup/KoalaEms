# Generated automatically from Makefile.in by configure.
# procs/sync/Makefile.in
# $ZEL: Makefile.in,v 1.1 2007/07/18 15:56:35 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = sync_proc.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

subdirs := $(sync)

SYNCTYPES = $(shell echo $(sync)|tr '[:lower:]' '[:upper:]')
SYNCFLAGS=$(foreach typ,$(SYNCTYPES),-DSYNC_$(typ) )
CFLAGS += $(SYNCFLAGS)
export SYNCFLAGS

lib = ../libprocs.a

all: $(lib)

#$(lib): $(lib)($(ofiles)) $(subdirs)
#	$(RANLIB) $@

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

$(subdirs): FORCE
	$(MAKE) -C $@ all

define MAKEIT
	$(MAKE) -C $(dir) depend

endef
MAKEDEP = $(foreach dir,$(lvd),$(MAKEIT))

depend: xdepend
	$(MAKEDEP)

.PHONY: FORCE depend
