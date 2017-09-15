*******************************************************************************
*                                                                             *
* chi_map.d                                                                   *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created before: 05.05.93                                                    *
* last changed: 05.05.93                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
*******************************************************************************

* special memories */
R_BASE     equ $00000000
M_BASE     equ $20000000
P_BASE     equ $48000000
F_BASE     equ $58000000
IO_BASE    equ 060000000
EA_BASE    equ $80000000
X_BASE     equ $C0000000
RTC_BASE   equ $E0000000
DUART_BASE equ $F0000000
BRAM_BASE  equ $E8000000
REG_BASE   equ $F8000000

* Basic set of primary address
DADR    equ  $0          /* offset to P_BASE+Arbitrationlevel */
CADR    equ  $4
DBRO    equ  $8
CBRO    equ  $c
AMS4    equ $10
AMS5    equ $14
AMS6    equ $18
AMS7    equ $1c
DGEO    equ $20
CGEO    equ $24
DBREG   equ $28
CBREG   equ $2c
AMS4EG  equ $30
AMS5EG  equ $34
AMS6EG  equ $38
AMS7EG  equ $3c

*Basic set of data cycles
RNDM    equ $4000       /* offset to P_BASE */
HBTOG   equ $4004
SECAD   equ $4008
PTOG    equ $400c
DMS4    equ $4010
DMS5    equ $4014
DMS6    equ $4018
DMS7    equ $401c

BHW     equ $4020
BHR     equ $4024
BPW     equ $4028
BPR     equ $402c

BSTOP   equ $4030
DISCON  equ $4034

* REG_BASE
POLLREG0 equ $0         /* relative to REG_BASE */
POLLREG1 equ $1         /* relative to REG_BASE */
IMASKREG equ $2         /* relative to REG_BASE */
LOCKREG  equ $4         /* relative to REG_BASE */
TREG     equ $6         /* relative to REG_BASE */
H_STATUS equ $7         /* relative to REG_BASE */
ASTATUS  equ $8         /* relative to REG_BASE */

* F_BASE
FSR      equ $0         /* relative to F_BASE */
FSR0     equ $0         /* relative to F_BASE */
FSR1     equ $1         /* relative to F_BASE */
FSR2     equ $2         /* relative to F_BASE */
FSR3     equ $3         /* relative to F_BASE */

FSRBUS0  equ $10        /* relative to F_BASE */
FSRBUS1  equ $11        /* relative to F_BASE */
FSRBUS2  equ $12        /* relative to F_BASE */
FSRBUS3  equ $13        /* relative to F_BASE */

RESGK   equ 0x50        /* relative to F_BASE */
FCLEAR  equ 0x60        /* relative to F_BASE */
GAREG    equ $100       /* relative to F_BASE */

*
FBRC0   equ $200        /* relative to F_BASE */
FBRC1   equ $201        /* relative to F_BASE */
FBRC2   equ $202        /* relative to F_BASE */

* IRQ_CTRL
LDM     equ $400        /* relative to F_BASE */
RDM     equ $404        /* relative to F_BASE */
CLRM    equ $408        /* relative to F_BASE */
SETM    equ $40c        /* relative to F_BASE */
BCLRM   equ $410        /* relative to F_BASE */
BSETM   equ $414        /* relative to F_BASE */
LDSTA   equ $418        /* relative to F_BASE */
RDSTA   equ $41c        /* relative to F_BASE */
ENIN    equ $420        /* relative to F_BASE */
DISIN   equ $424        /* relative to F_BASE */
RDVC    equ $428        /* relative to F_BASE */
CLRIN   equ $42c        /* relative to F_BASE */
CLRMR   equ $430        /* relative to F_BASE */
CLRMB   equ $434        /* relative to F_BASE */
CLRVC   equ $438        /* relative to F_BASE */
MCLR    equ $43c        /* relative to F_BASE */

* DMA
LDWC    equ $500        /* relative to F_BASE */
RDWC    equ $504        /* relative to F_BASE */
LDAC    equ $508        /* relative to F_BASE */
RDAC    equ $50c        /* relative to F_BASE */
WRCR    equ $510        /* relative to F_BASE */
RDCR    equ $514        /* relative to F_BASE */
REIN    equ $518        /* relative to F_BASE */
ENCT    equ $51c        /* relative to F_BASE */

OUT     equ $804        /* relative to F_BASE */
SSREG    equ $818       /* relative to F_BASE */
SSENABLE equ $b00       /* relative to F_BASE */

*FBRC0
FAS     equ $80
FGK     equ $40
OPEN    equ $20
BHSET   equ $10
B20RSET equ $08
PIP1    equ $04
PIP2    equ $02
PIP3    equ $01

* masks in LOCKREG
NOTIME  equ 0x80
SLOW    equ 0x40
LEVEL   equ 0x20
ENLEVEL equ 0x10
LATCH   equ 0x08
NOSLAVE equ 0x04
NOFB    equ 0x02
NOIO    equ 0x01

* SSENABLE
SSA7    equ $8000
SSA6    equ $4000
SSA5    equ $2000
SSA4    equ $1000
SSA3    equ $0800
SSA2    equ $0400
SSA1    equ $0200
SSA0    equ $0100

SSD7    equ $0080
SSD6    equ $0040
SSD5    equ $0020
SSD4    equ $0010
SSD3    equ $0008
SSD2    equ $0004
SSD1    equ $0002
SSD0    equ $0001

*******************************************************************************
*******************************************************************************
