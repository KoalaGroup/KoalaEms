# Generated automatically from Makefile.in by configure.
# lowlevel/Makefile.in
# $ZEL: Makefile.in,v 1.6 2004/06/18 16:46:41 wuestner Exp $

CONF = ../conf

srcdir = .


include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = devices.c lowlevel_init.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

lib = libll.a
subdirs = $(lowlevel)

ifdef lowlevel
all: $(lib)
else
all:
endif

LOWLEVELTYPES = $(shell echo $(lowlevel)|tr '[:lower:]/' '[:upper:]_')
LOWLEVELFLAGS=$(foreach typ,$(LOWLEVELTYPES),-DLOWLEVEL_$(typ) )
CFLAGS += $(LOWLEVELFLAGS)
export LOWLEVELFLAGS

$(lib): $(lib)($(ofiles)) $(subdirs) lowlevel.stamp
	$(RANLIB) $@

$(subdirs): FORCE
	$(MAKE) -C $@ all

lowlevel_init.c: lowlibs

lowlibs: lowlevel.stamp
	-$(RM) $@
	for l in $(lowlevel) ; do \
	echo "lowlib($$l)" >>$@ ; \
	done

depend:
	for i in $(subdirs) ; do \
	  echo checking directory $$i ; \
	  $(MAKE) -C $$i depend ; \
	done

.PHONY: FORCE depend

clean:
	-$(RM) lowlevel_init.c
	-$(RM) lowlibs

########################################################

lowlevel_init.o: errorcodes.h
