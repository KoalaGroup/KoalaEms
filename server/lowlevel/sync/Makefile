# Generated automatically from Makefile.in by configure.
# lowlevel/sync/Makefile.in
# $ZEL: Makefile.in,v 1.2 2009/12/08 14:24:30 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = sync.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

subdirs := $(sync)

SYNCTYPES = $(shell echo $(sync)|tr '[:lower:]' '[:upper:]')
SYNCFLAGS=$(foreach typ,$(SYNCTYPES),-DSYNC_$(typ) )
CFLAGS += $(SYNCFLAGS)
export SYNCFLAGS

lib = ../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles)) $(subdirs)
	$(RANLIB) $@

$(subdirs): FORCE
	$(MAKE) -C $@ all

define MAKEIT
	$(MAKE) -C $(dir) depend

endef
MAKEDEP = $(foreach dir,$(sync),$(MAKEIT))

depend: xdepend
	$(MAKEDEP)

.PHONY: FORCE depend
