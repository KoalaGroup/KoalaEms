# Generated automatically from Makefile.in by configure.
# lowlevel/camac/jorway73a/Makefile.in
# $ZEL: Makefile.in,v 1.5 2007/09/17 00:35:50 wuestner Exp $

CONF = ../../../conf

srcdir = .

include $(CONF)/rules.make
include $(CONF)/config.make
CFLAGS  += -DSJY73A

cfiles = jorway73a.c jorway73a_lam.c \
         sjy_cdchn.c sjy_cccd.c sjy_cccc.c sjy_ccci.c sjy_cccz.c sjy_cssa.c \
         sjy_cfsa.c sjy_cdreg.c sjy_cmsg.c sjy_ctgl.c sjy_cclm.c sjy_cgreg.c \
         sjy_cccbyp.c sjy_cccoff.c sjy_ctstat.c sjy_csmad.c sjy_cfmad.c \
	 sjy_csubc.c sjy_cfubc.c sjy_camerr.c sjy_cderr.c sjy_ccctype.c \
	 sjy_csuns.c sjy_xrespn.c sjy_qrespn.c sjy_get_qx.c sjy_cdroute.c \
	 sjy_reroute.c sjy_interface.c sjy_semops.c
ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

CFLAGS += $(CAMACFLAGS)

lib = ../../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

######################################################
