# $Id: useargs.tcl,v 1.1 2000/07/15 21:33:34 wuestner Exp $
# � 1999 P. W�stner; Zentrallabor f�r Elektronik; Forschungszentrum J�lich
#
# global vars:
#
# namedarg(socket)
# namedarg(host)
# namedarg(port)
# global_setup(commu_transport)
# global_setup(commu_socket)
# global_setup(commu_host)
# global_setup(commu_port)
# argv0
# 

proc use_args {} {
  global global_setup namedarg

  if [info exists namedarg(socket)] {
    set global_setup(commu_socket) $namedarg(socket)
    set global_setup(commu_transport) unix
  }

  if [info exists namedarg(host)] {
    set global_setup(commu_host) $namedarg(host)
    set global_setup(commu_transport) tcp
  }

  if [info exists namedarg(port)] {
    set global_setup(commu_port) $namedarg(port)
    set global_setup(commu_transport) tcp
  }
}

proc printhelp {} {
  global argv0
  puts ""
  puts "usage: $argv0 \[-help\] \[-socket <socket>\] \[-port <port>\]\
        \[-host <host>\]"
  puts "  -help prints this info"
  puts "  -socket <socket>: use unix domain socket to connect commu"
  puts "  -port <port>    : use port to connect commu"
  puts "  -host <host>    : connect to commu running on host"
}
