# Generated automatically from Makefile.in by configure.
# server/objects/Makefile.in
# $ZEL: Makefile.in,v 1.4 2002/11/06 15:37:00 wuestner Exp $

CONF = ../conf

srcdir = .


include $(CONF)/rules.make
include $(CONF)/config.make

ofiles = objectcommon.o notifystatus.o
depfiles = objectcommon.c notifystatus.c

lib = libobj.a
subdirs = ved $(objects)

all: $(lib)

$(lib): $(lib)($(ofiles)) $(subdirs) objects.stamp
	$(RANLIB) $@

$(subdirs): FORCE
	$(MAKE) -C $@ all

depend: xdepend
	for i in $(subdirs) ; do \
	  echo checking directory $$i ; \
	  $(MAKE) -C $$i depend ; \
	done

.PHONY: FORCE depend

###############################################################

