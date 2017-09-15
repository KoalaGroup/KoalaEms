/* Function-codes (nAF) fuer Scaler (Schlumberger JEA 20, aus Manual)   */

#define SCA_READ        0	/* nAFread(3,0,0)	*/ 	
#define SCA_WRITE       16	/* nAFwrite(3,0,16,x)	*/
#define SCA_RESET       9       /* nAFcntl(3,0,9)	*/
#define SCA_ENABLE      26	/* nAFcntl(3,0,26)	*/
#define SCA_DISABLE     24	/* nAFcntl(3,0,24)	*/
#define SCA_READ_RESET  2	/* nAFread(3,0,2)	*/
#define SCA_TEST_LAM    8	/* nAFcntl(3,0,8)	*/
#define SCA_CLEAR_LAM   10	/* nAFcntl(3,0,10)	*/
#define SCA_INCREMENT   25	/* nAFwrite(3,0,25,x)	*/
#define SCA_SUB_ADR     0       /* Subadresse (nicht benoetigt) */
