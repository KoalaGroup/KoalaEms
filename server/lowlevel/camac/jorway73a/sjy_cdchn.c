/* ************************************************************************* */
/* ===================================================================== 
**
** SUBROUTINE	CDCHN
**
** DESCRIPTION
**	Subroutine to initialize CAMAC environment
**	It opens the scsi device file and establish
**	the mapping between the object file and the user application.
**
** DEVELOPERS
**      Margherita Vittone
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
**      Eric G. Stern
**      Nevis Labs
**
** MODIFICATIONS
**         Date       Initials  Description
**      21-Aug-1991	MVW     Creation
**	24-Sep-1992	EGS	Modified for MVME147/167 and VxWorks
**	21-Oct-1992	DAS	Modified for VSD2992
**	01-Aug-1994	JAC	IRIX 5, VxWorks
**	06-Sep-1994	JMS	Add semaphore, >1 vsd
**	16-Sep-1994	JMS	JY411S
**	13-Sep-1995	DAS	Close sjy_file if channel=0
**	21-Sep-1995	DAS	Check fd status with ioctl before close 
**      05-Aug-1997     das     modified for Linux
**
**  ======================================================================== */
/*									     */

/*#define DEBUG*/

#define XDEFLG
#include "ieee_fun_types.h"		/* ieee_status declared in this file */
#undef XDEFLG

#include <sys/types.h>
#include <sys/stat.h>

int sjy_getdev (char *dev, int branch);


unsigned int cdchn (int	branch,
	            int	channel,
	            int	route)

{
/* -----------------------------------------------------------------------   */
  char	filename[72];
  char	version[6];
  int	sjy_file;
  int	status;
  int	scsi_id,scsi_bus;
  struct stat buf;
  char  dev[32];

  if(branch < 0 || branch > SJY_BRANCH_MAX)
	return(CAM_S_INVLD_BRANCH);

  /* close the SCSI device file */
  if (channel == 0) {  
    if (fstat(sjy_branch[branch].fd, &buf) != -1) {
      close(sjy_branch[branch].fd);
      return(CAM_S_SUCCESS);
    }
  return(CAM_S_SCSI_CLOSE_FAIL);
  }

  /* create the semaphore */
  if(sjy_inisem() == -1){
    perror("cdchn: inisem");
    return(CAM_S_SEM_FAIL);
  }

  /* find the scsi device name based on our scsi id */
  status = sjy_getdev (filename, branch);
  if (status != CAM_S_SUCCESS) {
    printf("CDCHN Error finding generic scsi device\n");
    return(status);
  }

  /* reserve the semaphore */
  if(sjy_getsem() != 0){
    perror("cdchn: getsem");
    sjy_putsem();
    return (CAM_S_GETSEM_FAIL);
  }

  /* open the scsi device file */
  if ((sjy_file = open(filename,O_RDWR) ) == -1) {
      printf("CDCHN Error opening %s\n",filename);
      perror("CDCHN");
      sjy_putsem();
      return (CAM_S_DEV_OPEN_FAIL);
      }

  /* release the semaphore */
  if(sjy_putsem() != 0) {
    perror("cdchn: putsem");
    return (CAM_S_PUTSEM_FAIL);
  }

#ifdef DEBUG
  printf("cdchn: filename = %s  fd = %d\n", filename, sjy_file);
#endif

  sjy_branch[branch].scsi_id 	=  branch & 0x0007;
  sjy_branch[branch].serial	= 1;		     /* assume serial */
  sjy_branch[branch].fd	        = sjy_file;

  /* Send two Test Unit Ready Commands to Jorway */
  status = sjy_tur(branch);
  if (status) {                        /* 0=success */
    printf("CDCHN Error executing Test Unit Ready\n");
    perror("cdchn");
    return (CAM_S_DEV_OPEN_FAIL);
  }

  return (CAM_S_SUCCESS);
}
/* -----------------------------------------------------------------------   */
/* ************************************************************************* */
/* ===================================================================== 
**
** SUBROUTINE	CDCHN_
**
** DESCRIPTION
**	Subroutine to initialize CAMAC environment
**	It opens the vme device file and establish
**	the mapping between the object file and the user application.
**
**	This stub is to make the INICAM call possible from a FORTRAN program
**
** DEVELOPERS
**      Margherita Vittone
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
** MODIFICATIONS
**         Date       Initials  Description
**      21-Aug-1991	MVW     Creation
**
**  ======================================================================== */
/*									     */
void cdchn_ (int	*branch,
              int	*channel,
              int	*route)
{
    ieee_status = cdchn(*branch,*channel,*route);
}

/* -----------------------------------------------------------------------   */
/* ************************************************************************* */
/* ===================================================================== 
**
** SUBROUTINE	sjy_getdev
**
** DESCRIPTION
**	Find the unknown (generic?) and Jorway devices in /proc/scsi/scsi 
**      and match our scsi id. Make a guess at what device (sga-sgh) was 
**      dynamically assigned by the system.
**	
**
** DEVELOPERS
**      Dave Slimmer
**      Fermilab Data Acquisition Group
**      Batavia, Il 60510, U.S.A.
**
** MODIFICATIONS
**         Date       Initials  Description
**      25-Aug-1991	DAS     Creation
**      26-Feb-1998	TS      count EACH existing SCSI device when
**                              reconstructing the device name
**
**  ======================================================================== */
/*									     */

static char *devtble[] =   {"/dev/sga",   /* scsi generic devices */
			    "/dev/sgb",
			    "/dev/sgc",
			    "/dev/sgd",
			    "/dev/sge",
			    "/dev/sgf",
/* TS */ /*
			    "/dev/sgh",
*/ /* TS */
			    "/dev/sgg",
			    "/dev/sgh"};

int sjy_getdev (char *dev, int scsiId)
{
	char charBuff[120];
	int charBuffsize = sizeof(charBuff);
	int status = CAM_S_SUCCESS;

	FILE *g_rfp;
        char filename[32];
	int id;
	char vendor[10];
        char type[10];
	int idx = 0;
	int found = 0;


	strcpy(filename, "/proc/scsi/scsi");
	
	g_rfp = fopen(filename, "r");
	if(g_rfp == NULL)
	{
		printf("sjy_getdev: Cannot open %s \n", filename);
		return(CAM_S_DEV_OPEN_FAIL);
	}

        /* read each line of file "filename" */
	while(status == CAM_S_SUCCESS)
	{
                
		status = readFile(g_rfp, filename, charBuff, charBuffsize);
		if (status == CAM_S_DEV_OPEN_FAIL)    /* file not found */
		{
			break;
		}
		if(status == 0)    /* end of file */
		{
		        if (!found) status = CAM_S_DEV_OPEN_FAIL;
			break;
		}
		if(status == CAM_S_SUCCESS)    /* parse the file line */
		{

		  /* get scsi id */
		  if (strncmp(charBuff, "Host", 4) == 0) {
		    sscanf(&charBuff[29],"%d",&id);
		    /*printf("found id %d\n", id);*/
		  }

		  /* get vendor */
		  if (strncmp(&charBuff[2], "Vendor", 6) == 0) {
		    sscanf(&charBuff[10],"%s", vendor);
		    /*printf("found vendor %s\n", vendor);*/
		    if (strcmp(vendor, "JORWAY") == 0) {
		      if (id == scsiId) {
		        /* found our device by matching scsi id */  
		        strcpy(dev, devtble[idx]);
			found = 1;
		        break;
		      } else {
		        idx++;
                      }
/* TS */
		    } else {
		      idx++;
/* TS */
		    }
		  }

		  /* get type and check for other (non JORWAY) generic scsi devices */
		  if (strncmp(&charBuff[2], "Type", 4) == 0) {
		    sscanf(&charBuff[10],"%s", type);
		    if ((strcmp(type, "Unknown") == 0) && (!strcmp(vendor, "JORWAY") == 0)) {
		      idx++;
		      /*printf("found Unknown\n");*/
		    }
 		  }
		    
		} /* status */

	}

	fclose(g_rfp);
	return(status);
}


/*
** readFile - Function to read file "filename" on stream g_rfp and put 
**            results into buffer charBuff.
**
*/

int readFile(FILE *g_rfp, char *filename, char charBuff[], int charBuffsize)
{

	int rVal;

	while(1)
	{
		if(fgets(charBuff, charBuffsize, g_rfp) != 0)
		{
		      strtok(charBuff, "\n");
		      rVal = CAM_S_SUCCESS;
		      break;
		} else {
		      rVal = 0;
		      break;
	        }

	}
	return(rVal);
}






