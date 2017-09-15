
/* Identificationnumber for the Commandword */
#define CLEAR       1
#define W_PARAM     2
#define START	    3
#define STOP	    4
#define READ        5
#define PAGE        6

/* Aktionen */
#define C_CLEAR     0x100
#define C_W_PARAM   0x200
#define C_START	    0x300
#define C_STOP	    0x400
#define C_READ      0x500
#define C_PAGE      0x600

/* Min and Max for the Commandword */
#define C_MIN	1
#define C_MAX	4

/* first eigth LowerBit X-Infomation and */
/* the next eigth LowerBit Y-Information */
#define XDETMIN 1
#define XDETMAX 8
#define YDETMIN 9
#define YDETMAX 16

#define PAGEMAXSIZE 0x400
 
/* Offsets in the Channelcard */
#define CARD_OFFSET 0x4000      /* Commandoffset */
#define MEMORY_OFFSET 0x6000    /* Memoryoffset  */
#define MEMORY_SIZE   0x40000   /* size of Memory*/
#define TIMEBIT_OFFSET 0xC      /* Timebitoffset */
#define INHIBIT_OFFSET 0x6      /* Inhibitoffset */
 
#define WAIT_MAX 400

/* Returncodes */
#define COMMAND_DONE 0x00
#define NOT_ACCEPT   0x01
#define NOT_DONE     0x02
#define BAD_HDW_ADDR 0x03

