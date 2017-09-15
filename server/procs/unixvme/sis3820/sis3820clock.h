/***************************************************************************/
/*                                                                         */
/*  Filename: sis3820clock.h                                               */
/*                                                                         */
/*  Funktion: headerfile for SIS3820 clock module                          */
/*                                                                         */
/*  Autor:                MKI                                              */
/*  last modification:    08.12.2003                                       */
/*                                                                         */
/* ----------------------------------------------------------------------- */
/*                                                                         */
/*  SIS  Struck Innovative Systeme GmbH                                    */
/*                                                                         */
/*  Harksheider Str. 102A                                                  */
/*  22399 Hamburg                                                          */
/*                                                                         */
/*  Tel. +49 (0)40 60 87 305 0                                             */
/*  Fax  +49 (0)40 60 87 305 20                                            */
/*                                                                         */
/*  http://www.struck.de                                                   */
/*                                                                         */
/*  © 2003                                                                 */
/*                                                                         */
/***************************************************************************/


/* addresses  */ 
#define SIS3820CLOCK_SOURCE					0x10			/* read/write; D32 */
#define SIS3820CLOCK_FREQUENCY					0x14			/* read/write; D32 */
#define SIS3820CLOCK_KEY_RESET					0x60			/* write only; D32 */


