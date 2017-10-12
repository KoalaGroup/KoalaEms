# $Id: run_commands.tcl,v 1.2 1999/04/09 12:32:39 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# global_verbose
#

proc call_run_commands {space t} {
  global global_daq
  set global_daq(t) $t

namespace eval vedsetup_$space {

  set t $global_daq(t)
  if {$global_verbose} {output "${t}-commands for VED $vedname ($description)"\
      tag_green}

  if [array exists ${t}_proclist] {
    foreach i [array names ${t}_proclist] {
      set header "${t}-commands for VED $vedname, ${t}_proclist"
      if {$global_verbose} {
        output_append "proclist for IS $i: [set ${t}_proclist($i)]"
      }
      if {$i==0} {
        set is_command $ved
      } else {
        if {![info exists is($i)]} {
          if {!$global_verbose} {output $header tag_red}
          output "there is no IS $i configured" tag_red
          return -1
        }
        set is_command $is($i)
      }
      if [catch {set res [$is_command command [set ${t}_proclist($i)]]} mist] {
        if {!$global_verbose} {output $header tag_red}
        output "IS $i: $mist" tag_red
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
      set is [lindex $vidx 0]
      if {$global_verbose} {
        output_append "command for IS $is: [set ${t}_command($i)]"
        if [info exists ${t}_args($i)] {
          output_append "  args for IS $is: [set ${t}_args($i)]"
        }
      }
      if {$is==0} {
        set is_command "nix"
      } else {
        if {![info exists is($is)]} {
          if {!$global_verbose} {output $header tag_red}
          output "there is no IS $is configured" tag_red
          return -1
        }
        set is_command $is($is)
      }
      if [catch {
        if [info exists ${t}_args($i)] {
          set res [eval [set ${t}_command($i)] $ved $is_command [set ${t}_args($i)]]
        } else {
          set res [eval [set ${t}_command($i)] $ved $is_command]
        }
      } mist] {
        if {!$global_verbose} {output $header tag_red}
        output "IS $is: [set ${t}_command($i)]: $mist" tag_red
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
    if [catch {set res [$ved command [set ${t}_proclist_t]]} mist] {
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
}
  return 0
}
