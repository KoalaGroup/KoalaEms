# Generated automatically from Makefile.in by configure.
# lowlevel/profile/Makefile.in
# $ZEL: Makefile.in,v 1.5 2007/09/17 00:35:51 wuestner Exp $

CONF = ../../conf

srcdir = .


include $(CONF)/rules.make
include $(CONF)/config.make

ofiles = profile.o $(profilelib).o

all: lib.a

lib.a: $(ofiles) profilelib.stamp
	ar rslc $@ $^
