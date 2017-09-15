#ifndef ican
#define ican

//#################################################################################################################
//Defines of the DATA-IDs from the iseg CAN device protokoll!

//Data_ID's for a message to a single channel of module - Single Instructions
#define DataID_ActualVoltage      0x80
#define DataID_ActualCurrent      0x90
#define DataID_SetVoltage         0xa0
#define DataExtID_SetCurrent      0xa0
#define DataID_Status             0xb0
#define DataExtID_CurrentTrip     0x80
#define DataExtID_SNominalValues  0x90
#define DataExtID_ActualVoltageTStamp	0xa0
#define DataExtID_ActualCurrentTStamp	0xb0

#define Channel_0                 0x00
#define Channel_1                 0x01
#define Channel_2                 0x02
#define Channel_3                 0x03
#define Channel_4                 0x04
#define Channel_5                 0x05
#define Channel_6                 0x06
#define Channel_7                 0x07
#define Channel_8                 0x08
#define Channel_9                 0x09
#define Channel_10                0x0a
#define Channel_11                0x0b
#define Channel_12                0x0c
#define Channel_13                0x0d
#define Channel_14                0x0e
#define Channel_15                0x0f

//Data_ID's for a message to all channels of module - Single Instructions
#define DataID_GenStatus          0xc0    // general status
#define DataID_StatusVLimit       0xc4    // status of voltage limit of all channels (bool array)
#define DataID_StatusILimit       0xc8    // status of hardware current limit of all channels (bool array)
#define DataID_OnOff              0xc8    // switch on or off via bool array for all channels
#define DataID_RampSpeed          0xd0    // ramp speed if the set voltage will changed of the channel will switched off
#define DataID_EmergencyOff       0xd4    // emergency off without ramp  via bool array for all channels
#define DataID_LogOn              0xd8    // log on which send after power on from the device
#define DataID_BitRate            0xdc    // please refer manual before write any bit rate
#define DataID_DID                0xe0    // serial number, software release and number of channel of the device
#define DataID_VsetAll            0xe4    // set voltage for all channels
#define DataID_hwILimit           0xe8    // hardware current limit
#define DataID_KillEnable         0xec    // set of kill enable/disable via bool array of all channels
#define DataID_Filter             0xf0    // value to change the convertion time (see manual)
#define DataID_NominalValues      0xf4    // nominal values of the module
#define DataID_swILimit           0xf8    // software current trip
#define DataID_Flash              0xfc    // reserved for service programs of iseg Spezialelektronik GmbH

#define DataExtID_Supply          0xc0
#define DataExtID_ChAllocation    0xc8
#define DataExtID_ChAviability    0xcc
#define DataExtID_SenseErr        0xd0
#define DataExtID_RelayMask       0xd4
#define DataExtID_ThErrStat       0xd8
#define DataExtID_CalPWM          0xdc
#define DataExtID_FErr	          0xe0
#define DataExtID_IsetForall      0xe4
#define DataExtID_HWVLimit	  0xe8

//device classes
#define Standard16bit              0x00
#define HighPrec24bit              0x01
#define Standard24bit              0x02
#define MultiVolt24bit             0x03
#define CAN_IO_RU                  0x04
#define CAN_IOStandard             0x05
#define Standard16bit4kV           0x06
#define Standard24bit4kV           0x07

#define NIMStandard                0x10
#define NIMHighPrecision           0x11


//flash instructions
#define     notready                   0
#define     ready                      1
#define     erase                     'E'
#define     enot	               2
#define     erased	               3
#define     prog	              'P'
#define     pnot	               4
#define     proged                     5
#define     noVfperr                   6

//Craete Data-IDs
#define     CDataID_IO1                0x41
#define     CDataID_AD00               0xa0
#define     CDataID_AD01               0xa1
#define     CDataID_AD02               0xa2
#define     CDataID_AD03               0xa3
#define     CDataID_AD04               0xa4
#define     CDataID_AD05               0xa5
#define     CDataID_AD06               0xa6
#define     CDataID_AD07               0xa7
#define     CDataID_GID                0xc6
#define     CDataID_SID                0xc6

//can open ids
#define     DBTID                      0x7e8
#define     NMTID                      0x7ea
#define     NmtStaStoRst
#define     canstart  0x01
#define     canstop   0x02
#define     canrst    0x04
#define     canrstdev 0x08
#define     canadjust 0x10

//NIM Data-ID
#define     DataID_StartVoltageCh      0x88
#define     DataID_HWLimitsNIM         0x98
#define     DataID_ITripNIM            0xa8
#define     DataID_GradNIM             0xb0
#define     DataID_ExtGradNIM          0xb4
#define     DataID_AutoStart           0xb8

#define     DataID_ModulStatus         0xc4
#define     DataID_LAMStatus           0xc8

//status modul
#define     hwVLimNoEx          b6      // hardware voltage limit has been exceeded
#define     killena             b6      // globale kill enable or disable
#define     supply	        b5      // supply error
#define     adjust	        b4	// adjustment yes or no
#define     filtfr		b3	// filter frequency large or small
#define     sloop		b2	// safety loop closed or open
#define     busy		b1	// ramping or stable
#define     StGes		b0	// sum bit error as status evaluation over all channels


#define     ChannelValue         16
#define     disconnect          0x00
#define     connect             0x01

#define     RTR                 0x001
#define     DLC	                0x0f
#define     DATA_DIR             1
#define     EXT_INSTR            2

//nmt
#define isegNmtAdr      0xc0
#define isegNmtStart    0xc4
#define isegNmtStop     0xc8
#define isegNmtRstCAN   0xcc
#define isegNmtRstHW    0xd0
#define isegNmtBtr      0xd4
#define isegNmtModTemp  0xd8

//quality
#define QUALITY_BAD     0
#define QUALITY_GOOD    192

//resolution
#define standard  50000         //50E+3
#define highprec  10000000      //10E+6

//keys
#define Enter           13
#define ESC             27

#define uw8    unsigned char
#define sw8    char
#define uw16   unsigned short
#define sw16   short
#define uw32   unsigned long
#define uw64   unsigned __int64

#define b0     0x01
#define b1     0x02
#define b2     0x04
#define b3     0x08
#define b4     0x10
#define b5     0x20
#define b6     0x40
#define b7     0x80

#define sp     0x20
#define esc    0x1b
#define enter  0x0d
#define lf     0x0a
#define bell   0x07
#define bs     0x08

#endif
