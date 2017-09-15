# Generated automatically from Makefile.in by configure.
# lowlevel/fastbus/sis3100_sfi/Makefile.in
# $ZEL: Makefile.in,v 1.6 2007/09/17 00:35:51 wuestner Exp $

CONF = ../../../conf

srcdir = .


include $(CONF)/rules.make
include $(CONF)/config.make

cfiles = \
    sis3100_sfi_init.c \
    sis3100_sfi_open_dev.c \
    sis3100_sfi_init_mapped.c \
    sis3100_sfi_reset.c \
    sis3100_sfi_delay_read.c \
    sis3100_sfi_read_delayed.c \
    sis3100_sfi_done.c \
    sis3100_sfi_arbitrationlevel.c \
    sis3100_sfi_FRcd.c \
    sis3100_sfi_multiFRcd.c \
    sis3100_sfi_FWcd.c \
    sis3100_sfi_FRcdB.c \
    sis3100_sfi_FRcdM.c \
    sis3100_sfi_FWcdM.c \
    sis3100_sfi_FCPcd.c \
    sis3100_sfi_FCDISC.c \
    sis3100_sfi_FCRWs.c \
    sis3100_sfi_FCWSA.c \
    sis3100_sfi_FCWWs.c \
    sis3100_sfi_IO.c \
    sis3100_check_balance.c \
    sis3100_sfi_read_fifo.c \
    sis3100_sfi_read_fifoword.c \
    sis3100_sfi_read_fifo_num.c \
    sis3100_sfi_wait_sequencer.c \
    sis3100_sfi_restart_sequencer.c \
    sis3100_sfi_rw.c \
    sis3100_sfi_multi_write.c \
    sis3100_sfi_FRDB_multi.c \
    sis3100_sfi_load_list.c

ofiles = $(cfiles:.c=.o)
depfiles = $(cfiles)

CFLAGS += $(FASTBUSFLAGS)

lib = ../../libll.a

all: $(lib)

$(lib): $(lib)($(ofiles))
	$(RANLIB) $@

depend: xdepend

.PHONY: depend

######################################################
