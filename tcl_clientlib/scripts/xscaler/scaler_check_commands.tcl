# $ZEL: scaler_check_commands.tcl,v 1.2 2006/08/13 17:32:04 wuestner Exp $
# copyright 1998
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_commands      // command definition from setup
# global_num_channels  // total number of channels
# global_comm_indices       indices of command definitions
# global_comm_ved(idx)      VED for command idx
# global_comm_is(idx)       IS
# global_comm_proc(idx)     0: EMS-command 1: procedure list
# global_comm_command(idx)  commandlist
# global_comm_callback(idx) callbacklist (pairs of command and channel number)
# global_comm_num(idx)      number of channels
# global_comm_offs(idx)     index of first channel
# global_comm_reqnum(soll)  number of requests per sequence
# 

proc check_commands {} {
  global global_num_channels
  global global_commands
  global global_comm_indices
  global global_comm_ved
  global global_comm_is
  global global_comm_proc
  global global_comm_command
  global global_comm_callback
  global global_comm_num
  global global_comm_offs
  global global_comm_reqnum

  set global_comm_indices [lsort -integer [array names global_commands]]
  set global_num_channels 0
  set global_comm_reqnum(soll) 0

  foreach i $global_comm_indices {
    if {[llength $global_commands($i)]!=5} {
      puts "wrong definition of command $i:"
      puts  $global_commands($i)
      return -1
    }
    set global_comm_ved($i) [lindex $global_commands($i) 0]
    set global_comm_is($i) [lindex $global_commands($i) 1]
    set c [lindex $global_commands($i) 2]
    if {($c!="c") && ($c!="p")} {
      puts "wrong type of command $i: $c"
      return -1
    }
    if {$c=="c"} {set global_comm_proc($i) 0} else {set global_comm_proc($i) 1}
    set global_comm_command($i) [lindex $global_commands($i) 3]
    set global_comm_callback($i) [lindex $global_commands($i) 4]
    set num 0
    foreach cc $global_comm_callback($i) {
      if {[llength $cc]!=2} {
        puts "wrong syntax of callback/num pair: $cc"
        return -1
      }
      incr num [lindex $cc 1]
    } 
    set global_comm_num($i) $num
    set global_comm_offs($i) $global_num_channels
    incr global_num_channels $num
    incr global_comm_reqnum(soll)
  }
  return 0
}
