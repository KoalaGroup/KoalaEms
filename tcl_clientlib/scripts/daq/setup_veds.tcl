# $ZEL: setup_veds.tcl,v 1.12 2009/03/29 19:59:55 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
# 
# global_verbose
# global_setupdata(names)
#

proc reset_ved {space} {
    global global_verbose

    if {[ved_setup_$space eval info exists noreset] && 
            [ved_setup_$space eval set noreset]} {
        return 0
    }

set res [ved_setup_$space eval {
  if {$global_verbose} {output "resetting VED $vedname" tag_green}
# ? reset readout
  if {$global_verbose} {output_append "reset readout"}
  if [catch {set ro_stat [ved readout status]} mist] {
      output "VED [ved name]:\n$mist" tag_red
      return -1
    } else {
      if {[lindex $ro_stat 0]!="inactive"} {
        # try to reset readout
        if [catch {ved readout reset} mist] {
          output "VED [ved name]:\n$mist" tag_red
          return -1
        }
      }
    }
# reset VED
  if {$global_verbose} {output_append "reset ved"}
  if [catch {ved reset} mist] {
    output "VED [ved name]:\n$mist" tag_red
    return -1
  }
  output_append "VED $vedname reset" tag_green
  update idletasks
  return 0
}]
  return $res
}

proc setup_ved {space} {
  global global_verbose

#output "setup_ved space=$space" tag_orange

    if {[ved_setup_$space eval info exists nosetup] && 
            [ved_setup_$space eval set nosetup]} {
        return 0
    }

set res [ved_setup_$space eval {
  if {$global_verbose} {output "configuring VED $vedname" tag_green}

# Domains
## Modullist
  if [info exists modullist] {
    if {$global_verbose} {output_append "load modullist $modullist"}
    if [catch {ved modullist create $modullist} mist] {
      output "VED [ved name]:\n$mist" tag_red
      return -1
    }
  }
## Trigger
  if [info exists trigger] {
    if {$global_verbose} {output_append "load trigger $trigger"}
    if [catch {eval ved trigger create $trigger} mist] {
      output "VED [ved name]:\n$mist" tag_red
      return -1
    }
  }
## LAM
# not yet

## Datain
  if [info exists datain] {
    if {[array exists datain]==0} {
      output "Setup for VED [ved name]: datain is not an array." tag_red
      return -1
    }
    foreach i [lsort -integer [array names datain]] {
      if {$global_verbose} {output_append "load datain $i $datain($i)"}
      if [catch {eval ved datain create $i $datain($i)} mist] {
        output "VED [ved name], datain $i:\n$mist" tag_red
        return -1
      }
    }
  }
## Dataout
  if [info exists dataout] {
    if {[array exists dataout]==0} {
      output "Setup for VED [ved name]: dataout is not an array." tag_red
      return -1
    }
    foreach i [lsort -integer [array names dataout]] {
      if {$global_verbose} {output_append "load dataout $i $dataout($i)"}
      if [catch {eval ved dataout create $i $dataout($i)} mist] {
        output "VED [ved name], dataout $i:\n$mist" tag_red
        return -1
      }
    }
    if [array exists dataout_autochange] {
      foreach i [lsort -integer [array names dataout_autochange]] {
        foreach j $dataout_autochange($i) {
          if [info exists dataout($j)] {
            if [catch {eval ved dataout enable $j 0} mist] {
              output "VED [ved name], disable dataout $i:\n$mist" tag_red
              return -1
            }
          } else {
            output "VED [ved name]: dataout $i for dataout_autochange($i) does not exist" tag_red
            return -1
          }
        }
      }
    }
  }
# Variables
  if [info exists var_init] {
    if {[array exists var_init]==0} {
      output "Setup for VED [ved name]: var_init is not an array." tag_red
      return -1
    }
  }
  if [info exists vars] {
    if {[array exists vars]==0} {
      output "Setup for VED [ved name]: vars is not an array." tag_red
      return -1
    }
    foreach i [lsort -integer [array names vars]] {
      if {$global_verbose} {output_append "create variable $i, size $vars($i)"}
      if [catch {ved var create $i $vars($i)} mist] {
        output "VED [ved name], variable $i:\n$mist" tag_red
        return -1
      }
      if [info exists var_init($i)] {
        if {$global_verbose} {output_append "write variable $i, $var_init($i)"}
        if [catch {ved var write $i $var_init($i)} mist] {
          output "VED [ved name], write variable $i:\n$mist" tag_red
          return -1
        }
      }
    }
  }
# Instrumentation Systems
  if [info exists isid] {
    if {[array exists isid]==0} {
      output "Setup for VED [ved name]: isid is not an array." tag_red
      return -1
    }
    foreach i [lsort -integer [array names isid]] {
      if {$global_verbose} {output_append "create is $i, id $isid($i)"}
      if [catch {set is($i) [create_is $i $isid($i)]} mist] {
        output "VED [ved name], is $i:\n$mist" tag_red
        return -1
      }
      # memberlist
      if [info exists memberlist($i)] {
        if {$global_verbose} {output_append "create memberlist $i $memberlist($i)"}
        if [catch {$is($i) memberlist create $memberlist($i)} mist] {
          output "VED [ved name], is $i, memberlist:\n$mist" tag_red
          return -1
        }
      }
    }
  }
# readoutprocedures
    foreach code [lsort -dictionary [array names readoutproc]] {
        if {$global_verbose} {
            output_append "create readoutproc for $code"
        }
        set codelist [split $code .]
        if {[llength $codelist]!=2} {
            output "Setup for VED [ved name]: wrong index for readoutproc ($code)" tag_red
            return -1
        }
        set isidx [lindex $codelist 0]
        set procidx [lindex $codelist 1]
        if {![info exists is($isidx)]} {
            output "Setup for VED [ved name]: no IS $isidx defined" tag_red
            return -1
        }
        if {![info exists readouttrigg($code)]} {
            output "Setup for VED [ved name]: no readouttrigg($code) given" tag_red
            return -1
        }

        set iscommand $is($isidx)
        set proc $readoutproc($code)
        set triggs $readouttrigg($code)
        set prior 0
        if {[info exists readoutprio($code)]} {
            set prior $readoutprio($code)
        }

        if {$global_verbose} {
            output_append "priority $prior"
            output_append "trigger $triggs"
            output_append "procedure $proc"
        }
        if [catch {$iscommand readoutlist create $prior $triggs $proc} mist ] {
            output "Setup for VED [ved name]: readoutproc $procidx for is $isidx:\n$mist" tag_red
            return -1
        }
    }

  output_append "VED $vedname: configuration loaded" tag_green
  update idletasks
  return 0
}]
  return $res
}

proc setup_veds {} {
  global global_setupdata

  set res 0

  foreach i $global_setupdata(names) {
    if [reset_ved $i] {set res -1}
  }
  if {$res} {return $res}

  if {[call_run_commands $global_setupdata(names) preinit 0]} {
    return -1
  }

  foreach i $global_setupdata(names) {
    if [setup_ved $i] {return -1}
  }

  if {[call_run_commands $global_setupdata(names) init 0]} {
    return -1
  }
  if {[call_run_commands $global_setupdata(names) postinit 0]} {
    return -1
  }
  
  return $res
}

proc reset_veds {} {
  global global_setupdata

  set res 0
  foreach i $global_setupdata(names) {
    if [reset_ved $i] {set res -1}
  }
  return $res
}
