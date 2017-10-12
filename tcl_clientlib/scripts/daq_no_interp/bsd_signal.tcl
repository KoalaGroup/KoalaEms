# $Id: bsd_signal.tcl,v 1.2 1999/04/09 12:32:30 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# global_bsd_signal_set
# global_bsd_signal(1..81)
#

set global_bsd_signal_set 0

proc set_bsd_signal {} {
  global global_bsd_signal_set
  global global_bsd_signal
  array set global_bsd_signal {
   1  {terminal line hangup}
   2  {interrupt program}
   3  {quit program}
   4  {illegal instruction}
   5  {trace trap}
   6  {abort}
   7  {emulate instruction executed}
   8  {floating-point exception}
   9  {kill program}
   10  {bus error}
   11  {segmentation violation}
   12  {system call given invalid argument}
   13  {write on a pipe with no reader}
   14  {real-time timer expired}
   15  {software termination signal}
   16  {urgent condition present on socket}
   17  {stop}
   18  {stop signal generated from keyboard}
   19  {continue after stop}
   20  {child status has changed}
   21  {background read attempted from control terminal}
   22  {background write attempted to control terminal}
   23  {I/O is possible on a descriptor}
   24  {cpu time limit exceeded}
   25  {file size limit exceeded}
   26  {virtual time alarm}
   27  {profiling timer alarm}
   28  {Window size change}
   29  {status request from keyboard}
   30  {User defined signal 1}
   31  {User defined signal 2}
  }
  set global_bsd_signal_set 1
}

proc strsignal_bsd {signal} {
  global global_bsd_signal_set
  global global_bsd_signal
  if {!$global_bsd_signal_set} set_bsd_signal
  if [info exists global_bsd_signal($signal)] {
    return $global_bsd_signal($signal)
  } else {
    return "Signal $signal"
  }  
}
