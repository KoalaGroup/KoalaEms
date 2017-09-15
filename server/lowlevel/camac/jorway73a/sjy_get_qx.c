/*	fill Q and X, return Q			jms 7nov94 */

#include "ieee_fun_types.h"

int sjy_get_qx(int branch){

unsigned char	*buf;
int    *perrno = &sjy_branch[branch].errn;

if(*perrno == 0){			/* all OK */
	ieee_last_Q=1;
	ieee_last_X=1;
	}
else if(*perrno == ENODEV){	/* non data command, no Q */
	ieee_last_Q=0;
	ieee_last_X=1;
	}
else if(*perrno == EPIPE ){	/* bad word count transfer on pdt */
	ieee_last_Q=0;
	ieee_last_X=0;
	}
else if(*perrno == EIO ){	/* SCSI error */
	ieee_last_Q=0;
	ieee_last_X=0;
	}
else if(*perrno == EFAULT){	/* CAMAC error */
	ieee_last_Q=0;
	ieee_last_X=0;
	}
else{
        buf = sjy_reqSense (sjy_branch[branch].fd);
	ieee_last_Q= 1 - (buf[8]&128)/128;
	ieee_last_X= 1 - (buf[8]&64)/64;
	}

return(ieee_last_Q);
}






