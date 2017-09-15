# Generated automatically from Makefile.in by configure.
# lowlevel/lvd/vertex/Makefile.in
# $ZEL: Makefile.in,v 1.5 2008/06/19 15:10:48 trusov Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = lvd_vertex.c  vertex.c vertex_dump.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

LVDTYPES = $(shell echo $(lvd)|tr '[:lower:]' '[:upper:]')
LVDFLAGS=$(foreach typ,$(LVDTYPES),-DLVD_$(typ) )
CFLAGS += $(LVDFLAGS)
export LVDFLAGS

lib = ../../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend
