#if !defined(__icanHVapi_h)
#define __icanHVapi_h
#endif

#define icanHV_bufsize	20
u_int32 buf[icanHV_bufsize];

int icanHV_init();
int	icanHV_exit();
int icanHV_io(uint32 (*buf)[icanHV_bufsize]);

/*
word 0	0	nothing, ignore
		1	test, produce comments for log
		2	
		10	init
		11	quit
		12	(reserved for reset)
		15	test with CAN
		20	send/receive raw data bytes (LSB)
	word 1	number of bytes including word 1
	word 2	CAN-ID
	word 3..(icanHV_bufsize - 1)	Data
		21	send/receive voltage ramp
	word 1	CAN-ID
	word 2	time in miliseconds
	word 3	voltage in milivolt
		22	set several flags
	word 1	0	nothing, ignore
			1	test
			10	set
			11	test x
			12	test y
			100	nothing, ignore
			101	test
			102	set xxx
		23	read flags
		24	read voltages
		25	read currencies
*/