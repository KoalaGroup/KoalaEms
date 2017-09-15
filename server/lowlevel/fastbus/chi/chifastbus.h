/******************************************************************************
*                                                                             *
* lowlevel/chi/chifastbus.h                                                   *
*                                                                             *
* created: 25.04.97                                                           *
* last changed: 25.04.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _chifastbus_h_
#define _chifastbus_h_

#define CPY_TO_FBBUF_should_be_avoided(dest, source, num) \
(memcpy((int*)FB_BUFBEG+(dest), (source), (num)*4), ((int*)FB_BUFBEG+dest))

#define CPY_FROM_FBBUF_should_be_avoided(void* dest, SOURCE, num)


#endif

/*****************************************************************************/
/*****************************************************************************/
