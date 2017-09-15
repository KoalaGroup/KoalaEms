# $ZEL: run_commands.tcl,v 1.10 2009/05/25 17:10:40 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_verbose
#

proc set_run_nr {space runnr} {
    ved_setup_$space eval set runnr $runnr
    ved_setup_$space eval {
        if {[catch {ved command1 set_runnr $runnr}]} {
            output "don't wrote runnr $runnr to [ved name]"
	} else {
            output "wrote runnr $runnr to [ved name]"
	}
    }
    return 0
}

proc call_run_commands_sync {space t} {

$space eval set t $t
set res [$space eval {
    global global_verbose

  if {$global_verbose} {output "${t}-commands for VED $vedname ($description)"\
      tag_green}

  if [array exists ${t}_proclist] {
    foreach i [lsort -dictionary [array names ${t}_proclist]] {
      set header "${t}-commands for VED $vedname, ${t}_proclist"
      if {$global_verbose} {
        output_append "proclist for IS $i: [set ${t}_proclist($i)]"
      }
      set ii [lindex [split $i .] 0]
      if {$ii==0} {
        set is_command ved
      } else {
        if {![info exists is($ii)]} {
          if {!$global_verbose} {output $header tag_red}
          output "there is no IS $ii configured" tag_red
          return -1
        }
        set is_command $is($ii)
      }
      if [catch {set res [$is_command command [set ${t}_proclist($i)]]} mist] {
        if {!$global_verbose} {output $header tag_red}
        output "IS $ii: $mist" tag_red
        return -1
      }
      if {$res!=""} {
        if {!$global_verbose} {output $header}
        output_append "  returns $res"
      }
    }
  }
  if [array exists ${t}_command] {
    foreach i [array names ${t}_command] {
      set header "${t}-commands for VED $vedname, ${t}_command"
      set vidx [split $i .]
      set idx [lindex $vidx 0]
      if {$global_verbose} {
        output_append "command for IS $vidx: [set ${t}_command($i)]"
        if [info exists ${t}_args($i)] {
          output_append "  args for IS $vidx: [set ${t}_args($i)]"
        }
      }
      if {$idx==0} {
        set is_command "nix"
      } else {
        if {![info exists is($idx)]} {
          if {!$global_verbose} {output $header tag_red}
          output "there is no IS $idx configured" tag_red
          return -1
        }
        set is_command $is($idx)
      }
      if [catch {
        if [info exists ${t}_args($i)] {
          set res [eval [set ${t}_command($i)] ved $is_command [set ${t}_args($i)]]
        } else {
          set res [eval [set ${t}_command($i)] ved $is_command]
        }
      } mist] {
        if {!$global_verbose} {output $header tag_red}
        output "IS $idx: [set ${t}_command($i)]: $mist" tag_red
        if {$errorCode!="NONE"} {output_append "errorCode: $errorCode" tag_blue}
        output_append $errorInfo tag_blue
        return -1
      }
      if {$res} {return -1}
    }
  }
  if [info exists ${t}_proclist_t] {
    set header "${t}-commands for VED $vedname, ${t}_proclist_t"
    if {$global_verbose} {
      output_append "proclist_t: [set ${t}_proclist_t]"
    }
    if [catch {set res [ved command [set ${t}_proclist_t]]} mist] {
      if {!$global_verbose} {output $header tag_red}
      output "$mist" tag_red
      return -1
    }
    if {$res!=""} {
      if {!$global_verbose} {output $header}
      output_append "  returns $res"
    }
  }
  output_append "VED $vedname: $t procedures executed" tag_green
  update idletasks
  return 0
}]
  return $res
}

proc call_run_commands {names t ignore} {
    global global_setup
    global global_verbose

    if {$global_verbose} {
        output "call_run_commands $names $t" tag_green
        output_append "run_commands_async is $global_setup(run_commands_async)" tag_green
    }

    set res 0
    if {$global_setup(run_commands_async)} {
        set res [call_run_commands_async $names $t]
    } else {
        foreach name $names {
            if {[call_run_commands_sync $name $t]} {
                set res -1
                if {!$ignore} {
                    break
                }
            }
        }
    }
    return $res
}
