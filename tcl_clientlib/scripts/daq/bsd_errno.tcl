# $ZEL: bsd_errno.tcl,v 1.3 2003/02/04 19:27:48 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_bsd_errno_set
# global_bsd_errno(1..81)
#

set global_bsd_errno_set 0

proc set_bsd_errno {} {
  global global_bsd_errno_set
  global global_bsd_errno
  array set global_bsd_errno {
   1      {Operation not permitted}
   2      {No such file or directory}
   3      {No such process}
   4      {Interrupted system call}
   5      {Input/output error}
   6      {Device not configured}
   7      {Argument list too long}
   8      {Exec format error}
   9      {Bad file descriptor}
   10     {No child processes}
   11     {Resource deadlock avoided}
   12     {Cannot allocate memory}
   13     {Permission denied}
   14     {Bad address}
   15     {Block device required}
   16     {Device busy}
   17     {File exists}
   18     {Cross-device link}
   19     {Operation not supported by device}
   20     {Not a directory}
   21     {Is a directory}
   22     {Invalid argument}
   23     {Too many open files in system}
   24     {Too many open files}
   25     {Inappropriate ioctl for device}
   26     {Text file busy}
   27     {File too large}
   28     {No space left on device}
   29     {Illegal seek}
   30     {Read-only file system}
   31     {Too many links}
   32     {Broken pipe}
   33     {Numerical argument out of domain}
   34     {Result too large}
   35     {Resource temporarily unavailable}
   36     {Operation now in progress}
   37     {Operation already in progress}
   38     {Socket operation on non-socket}
   39     {Destination address required}
   40     {Message too long}
   41     {Protocol wrong type for socket}
   42     {Protocol not available}
   43     {Protocol not supported}
   44     {Socket type not supported}
   45     {Operation not supported}
   46     {Protocol family not supported}
   47     {Address family not supported by protocol family}
   48     {Address already in use}
   49     {Can't assign requested address}
   50     {Network is down}
   51     {Network is unreachable}
   52     {Network dropped connection on reset}
   53     {Software caused connection abort}
   54     {Connection reset by peer}
   55     {No buffer space available}
   56     {Socket is already connected}
   57     {Socket is not connected}
   58     {Can't send after socket shutdown}
   59     {Too many references: can't splice}
   60     {Operation timed out}
   61     {Connection refused}
   62     {Too many levels of symbolic links}
   63     {File name too long}
   64     {Host is down}
   65     {No route to host}
   66     {Directory not empty}
   67     {Too many processes}
   68     {Too many users}
   69     {Disc quota exceeded}
   70     {Stale NFS file handle}
   71     {Too many levels of remote in path}
   72     {RPC struct is bad}
   73     {RPC version wrong}
   74     {RPC prog. not avail}
   75     {Program version wrong}
   76     {Bad procedure for program}
   77     {No locks available}
   78     {Function not implemented}
   79     {Inappropriate file type or format}
   80     {Authentication error}
   81     {Need authenticator}
   81     {Must be equal largest errno}
  }
  set global_bsd_errno_set 1
}

proc strerror_bsd {errno} {
  global global_bsd_errno_set
  global global_bsd_errno
  if {!$global_bsd_errno_set} set_bsd_errno
  if [info exists global_bsd_errno($errno)] {
    return $global_bsd_errno($errno)
  } else {
    return "Error $errno"
  }  
}
