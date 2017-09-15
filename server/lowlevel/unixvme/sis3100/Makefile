# Generated automatically from Makefile.in by configure.
# lowlevel/unixvme/sis3100/Makefile.in
# $ZEL: Makefile.in,v 1.5 2007/09/17 00:35:51 wuestner Exp $

CONF = ../../../conf

srcdir = .


include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = vme_sis.c dsp_sis.c mem_sis.c load_dsp.c \
         read_dsp_coff.c read_dsp_ldr.c read_dsp_raw.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

CFLAGS += $(VMEFLAGS)

lib = ../../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend
