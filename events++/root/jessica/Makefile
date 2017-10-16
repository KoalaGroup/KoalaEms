# $ZEL: Makefile,v 1.1 2004/02/11 12:37:37 wuestner Exp $

#ROOTSYS      := /usr/local/root/root
ROOTINC      := $(ROOTSYS)/include
ROOTLIB      := $(ROOTSYS)/lib
CC           := gcc
CXX          := g++
CXXFLAGS     := -I$(ROOTINC) -g

LDFLAGS      := -L$(ROOTLIB) \
    -lCore \
    -lMatrix \
    -lGui \
    -lGpad \
    -lGraf \
    -lHist \
    -lHistPainter \
    -lCint \
    -lm \
    -ldl

.PHONY: all
all: spect_offline spect_online diff_spect

spect_offline: spect_offline.o spect_decode.o
	$(CXX) $(LDFLAGS) $^ -o $@

spect_online: spect_online.o spect_decode.o
	$(CXX) $(LDFLAGS) $^ -o $@

diff_spect: diff_spect.o spect_decode.o
	$(CXX) $(LDFLAGS) $^ -o $@

clean:
	rm -f *.o core $(EXEC)

