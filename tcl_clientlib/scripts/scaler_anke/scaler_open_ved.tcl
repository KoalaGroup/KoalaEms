#
# open_ved oeffnet ein VED und versucht eine Fehlerbehandlung
#
# return: 0: ok.
#        -1: schlug fehl, aber spaeter wieder moeglich
#        -2: schlug fehl (fatal)
#

proc open_ved {name var args} {
  global global_setup
  upvar $var ved

  output "connect to $name $args ..."
  update idletasks
  set retry 1
  set res -1
  while {$retry && $res<0} {
    if [catch {ems_open $name} mist] {
      output_append $mist
      update idletasks
      switch -exact $global_setup(policy_ved) {
        ignore {
          set res 0
          set ved {}
        }
        ask {
          set ares [tk_dialog .dialog {VED error} \
            "$mist" {} 0 \$global_setup(iss)
            abort retry {retry later}]
          switch -exact $ares {
            0 {set res -2; set retry 0; set ved {}}
            1 {set res -1}
            2 {set res -1; set retry 0; set ved {}}
          }
        }
      }
      update idletasks
    } else {
      set res 0
      set ved $mist
      output_append "ok."
      update idletasks
    }
  }
  return $res
}

proc unique {var} {
  set svar [lsort $var]
  set hvar ""
  set uvar {}
  foreach i $svar {
    if {"$i"!="$hvar"} {
      lappend uvar $i
      set hvar $i
    }
  }
  return $uvar
}

proc close_veds {} {
  set veds [ems_openveds]
  while {[llength $veds]} {
    output "close VED [lindex $veds 0]"
    update idletasks
    [lindex $veds 0] close
    set veds [ems_openveds]
  }
}

proc open_veds {} {
  global global_setup global_veds global_openveds\
    global_ved_of_is global_is_of_is

# VEDnamen einsammeln
# zuerst fuer Eventrate
  set veds $global_setup(evrate_ved)
# dann fuer Scaler
  foreach i $global_setup(iss) {
    lappend veds $global_ved_of_is($i)
  }
  if {[llength $veds]==0} {
    output "Keine VEDs zu oeffnen; alle Arbeit ist getan"
    return -1
  }
  set global_veds [unique $veds]

  foreach ved $global_veds {
    set res [open_ved $ved global_openveds($ved)]
    if {$res!=0} {
      close_veds
      return $res
    }
  }
  return 0
}
