/* Eventmanager-Datenoutput fuer OS-9 */

%define SOCKETHANDLER "insocket"
%define LOCALSOCKETHANDLER "inlocalsocket"
%define FILEHANDLER "infile"
%define TAPEHANDLER "aufexa"
%define NULLHANDLER "hauwech"
%define HANDLERCOMNAME "handlercom"

%define HANDLERCOMSIZE (EVENT_MAX*4)

%define OBJ_DO

%define DATAOUT_MULTI

objects += DO
dataout = RINGBUFFERS
