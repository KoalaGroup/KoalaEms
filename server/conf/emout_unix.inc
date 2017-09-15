/******************************************************************************
*                                                                             *
* emout_unix.config                                                           *
*                                                                             *
* 19.03.97                                                                    *
*                                                                             *
******************************************************************************/


%define SOCKETHANDLER "insocket"
%define LOCALSOCKETHANDLER "inlocalsocket"
%define FILEHANDLER "infile"
%define TAPEHANDLER "aufexa"
%define NULLHANDLER "hauwech"
%define HANDLERCOMNAME "handlercom"

%define HANDLERCOMSIZE (EVENT_MAX*4)

%define obj_do

%define DATAOUT_MULTI

objects += do
dataout = ringbuffers
