# Generated automatically from Makefile.in by configure.
# lowlevel/lvd/Makefile.in
# $ZEL: Makefile.in,v 1.9 2009/12/03 00:02:29 wuestner Exp $

CONF = ../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = lvd.c lvdbus.c lvd_controller.c lvd_access.c \
        lvd_jtag.c lvd_dumpstatus.c lvd_sync_statist.c lvd_i2c.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

subdirs := $(lvd)

LVDTYPES = $(shell echo $(lvd)|tr '[:lower:]' '[:upper:]')
LVDFLAGS=$(foreach typ,$(LVDTYPES),-DLVD_$(typ) )
CFLAGS += $(LVDFLAGS)
export LVDFLAGS

lib = ../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles)) $(subdirs)
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
