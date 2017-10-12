HEADERS = dspcode.h
HELPERS = read_dsp_coff.c read_dsp_ldr.c read_dsp_raw.c

all: dspload1 dspload2

dspload1: dspload1.c ${HELPERS} ${HEADERS}
	$(CC) ${CFLAGS} -o dspload1 dspload1.c ${HELPERS}

dspload2: dspload2.c ${HELPERS} ${HEADERS}
	$(CC) ${CFLAGS} -o dspload2 dspload2.c ${HELPERS}
