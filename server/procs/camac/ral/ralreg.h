#define RAL_MAX_ROWS 64
#define RAL_MAX_COLUMNS 16
#define RAL_FIFO_LEN 4096
#define RAL_FRAM_LEN 0x04000  /* filter RAM length */

#define camacdev(n) (n)->address.camac.dev
#define camacslot(n) (n)->address.camac.slot

#define RAL_RD_FIFO(n)	    camacdev(n)->CAMACaddr(camacslot(n), 0, 0)
#define RAL_TST_FIFO(n)     camacdev(n)->CAMACaddr(camacslot(n), 0, 27)
#define RAL_WR_DATA(n)      camacdev(n)->CAMACaddr(camacslot(n), 0, 16)
#define RAL_WR_STA1(n)	    camacdev(n)->CAMACaddr(camacslot(n), 0, 17)
#define RAL_WR_STA2(n)	    camacdev(n)->CAMACaddr(camacslot(n), 1, 17)
#define RAL_GEN_CLR(n)	    camacdev(n)->CAMACaddr(camacslot(n), 0, 9)
#define RAL_TST_LAM(n)	    camacdev(n)->CAMACaddr(camacslot(n), 0, 8)
#define RAL_TSTCL_LAM(n)    camacdev(n)->CAMACaddr(camacslot(n), 0, 10)
#define RAL_START(n)	    camacdev(n)->CAMACaddr(camacslot(n), 0, 25)
#define RAL_CLOCK(n)	    camacdev(n)->CAMACaddr(camacslot(n), 1, 25)
#define RAL_STROBE(n)	    camacdev(n)->CAMACaddr(camacslot(n), 2, 25)
#define RAL_RESET_(n)	    camacdev(n)->CAMACaddr(camacslot(n), 3, 25)
#define RAL_WR_RAM(n)	    camacdev(n)->CAMACaddr(camacslot(n), 1, 16)
#define RAL_RD_RAM(n)	    camacdev(n)->CAMACaddr(camacslot(n), 1, 27)
#define RAL_TST_BUSY(n)	    camacdev(n)->CAMACaddr(camacslot(n), 2, 27)
#define RAL_TST_FULL(n)	    camacdev(n)->CAMACaddr(camacslot(n), 3, 27)
#define RAL_WR_VSN(n)	    camacdev(n)->CAMACaddr(camacslot(n), 2, 16)

/* STATUS 1 */
#define RAL_TRO    0x01
#define RAL_EFB    0x02
#define RAL_ELAM   0x04
#define RAL_BWS    0x08
#define RAL_LPOS   0x10
#define RAL_DZS    0x20

/* STATUS 2 */
#define RAL_CTL_TST_ONE  1
#define RAL_CTL_LD_TST   2
#define RAL_CTL_TST_ZERO 3
#define RAL_CTL_LD_DAC   4
#define RAL_CTL_ROUT     5
#define RAL_CTL_RESET    7
#define RAL_DXST      0x10
#define RAL_STP_FCLK  0x20

/* FIFO DATA */
#define B_ROW     4
#define M_ROW     0x3F
#define B_COLUMN  10
#define M_COLUMN  0x0F
#define B_NULL    16
#define B_NEXT    17
