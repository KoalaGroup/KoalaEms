# $Id: write_setup.tcl,v 1.14 2000/08/31 15:43:13 wuestner Exp $
# copyright 1998
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_verbose               boolean
# global_setupdata(names)
# global_setupdata(em_ved)
# global_daq(_starttime)
# global_daq(_run_nr)
# global_daq(_current_run_nr)
# global_daq(_stoptime)
# global_init(init_done)
# global_superheader           string containing superheader
# global_configheader
# global_userheader
# global_noteheader
# global_namespaces($ved)   ; namespacename fuer alle veds
# global_headers_valid      ; status der generierten header
#

proc create_superheader {} {
  global global_superheader global_setupdata global_daq global_setup

  set time $global_daq(_starttime)
  set global_superheader {}
  lappend global_superheader "Key: superheader"
  lappend global_superheader "Facility: $global_setup(facility)"
  lappend global_superheader "Location: $global_setup(location)"
  lappend global_superheader "Run: $global_daq(_run_nr)"
  set global_daq(_current_run_nr) $global_daq(_run_nr)
  lappend global_superheader "Devices: $global_setupdata(names)"
  lappend global_superheader "Start Time: [clock format $time]"
  lappend global_superheader "Start Time_coded: $time"
}
proc update_superheader {} {
  global global_superheader global_daq
  set time $global_daq(_stoptime)
  lappend global_superheader "Stop Time: [clock format $time]"
  lappend global_superheader "Stop Time_coded: $time"
}
proc create_configheader {name} {
  global global_configheader
  set global_configheader($name) {}
  lappend global_configheader($name) "Key: configuration/$name"
  set ::vedsetup_${name}::name $name

  namespace eval vedsetup_$name {
    upvar #0 ::global_configheader($name) head

    lappend head "vedname $vedname"
    if [info exists eventbuilder] {
      lappend head "eventbuilder $eventbuilder"
    }
    if [info exists triggermaster] {
      lappend head "triggermaster $triggermaster"
    }
    if [array exists dataout] {
      foreach i [array names dataout] {
        lappend head "dataout($i) $dataout($i)"
      }
    }
    if [array exists datain] {
      foreach i [array names datain] {
        lappend head "datain($i) $datain($i)"
      }
    }
    if [info exists modullist] {
      lappend head "modullist $modullist"
    }
    if [array exists isid] {
      foreach i [array names isid] {
        lappend head "isid($i) $isid($i)"
      }
    }
    if [array exists memberlist] {
      foreach i [array names memberlist] {
        lappend head "memberlist($i) $memberlist($i)"
      }
    }
    if [array exists readouttrigg] {
      foreach i [array names readouttrigg] {
        lappend head "readouttrigg($i) $readouttrigg($i)"
      }
    }
    if [array exists readoutprio] {
      foreach i [array names readoutprio] {
        lappend head "readoutprio($i) $readoutprio($i)"
      }
    }
    if [array exists readoutproc] {
      foreach i [array names readoutproc] {
        lappend head "readoutproc($i) $readoutproc($i)"
      }
    }
    if [info exists trigger] {
      lappend head "trigger $trigger"
    }
    if [array exists vars] {
      foreach i [array names vars] {
        lappend head "vars($i) $vars($i)"
      }
    }
    if [array exists var_init] {
      foreach i [array names var_init] {
        lappend head "var_init($i) $var_init($i)"
      }
    }
  }
}
proc create_userheader {} {
  global global_userheader global_setupdata
  array set global_userheader {}
  set global_userfiles {}
  foreach i [lsort -integer [array names ::super_setup_sources]] {
    lappend global_userfiles [lindex [set ::super_setup_sources($i)] 0]
  }
  foreach n $global_setupdata(names) {
    foreach i [lsort -integer [array names vedsetup_${n}::setup_sources]] {
      lappend global_userfiles [lindex [set vedsetup_${n}::setup_sources($i)] 0]
    }
    if [info exists vedsetup_${n}::files_to_be_saved] {
      foreach i vedsetup_${n}::files_to_be_saved {
        lappend global_userfiles [set vedsetup_${n}::files_to_be_saved]
      }
    }
  }
  set idx 0
  foreach n $global_userfiles {
    set res [create_header_from_file $n global_userheader($idx)]
    if {$res<0} {
      array unset global_userheader $idx
    } else {
      incr idx
    }
  }
}

proc create_noteheader {} {
  global global_noteheader
  set global_noteheader {}
  lappend global_noteheader {Key: run_notes}
  lappend global_noteheader \{[enter_notes]\}
}

proc ivalidate_headers {} {
  global global_headers_valid
  set global_headers_valid invalid
}

proc dump_header {header} {
  output "headerdump:" tag_orange
  foreach i $header {
    output $i tag_orange
  }
}

proc write_header_to_tape {ved do header verb} {
  global errorCode global_waitvar global_waitproc
  global global_verbose
  set weiter 1
  set second_try 0
#   if {$verb} {
#     dump_header $header
#   }
  if {$global_verbose} output "write header: [lindex $header 0]"
  while {$weiter} {
    if [catch {$ved dataout write $do $header} mist] {
      output "write inforecord to dataout $do of VED [$ved name]: $mist" tag_red
      if {$errorCode=="EMS_REQ 38" || $errorCode=="EMS_REQ 35"} {
        output_append "  will retry in 5 seconds"
        set global_waitvar 0
        set global_waitproc [after 5000 {wait_handler}]
        vwait global_waitvar
        if {$global_waitvar!=1} {
          output "  abgewürgt"
          set res -1
          set weiter 0
        } else {
          set second_try 1
        }
      } else {
        output "errorCode=$errorCode"
        if {[llength $errorCode]>=3} {
          # XXX
          output_append "([strerror_bsd [lindex $errorCode 2]])"
        }
        set res -1
        set weiter 0
      }
    } else {
      if {$second_try} {output_append "  ...successfully"}
      set res 0
      set weiter 0
    }
  }
  return $res
}

proc write_setup_to_dataout {ved do} {
  global global_superheader global_configheader
  global global_noteheader global_userheader
  global global_verbose

  if {$global_verbose} {
    output "write header to ved [$ved name] dataout $do"
  }
  set res [write_header_to_tape $ved $do $global_superheader 1]
  if {$res!=0} {return -1}
  foreach h [array names global_configheader] {
    set res [write_header_to_tape $ved $do $global_configheader($h) 0]
    if {$res!=0} {return -1}
  }
  if [info exists global_noteheader] {
    set res [write_header_to_tape $ved $do $global_noteheader 0]
    if {$res!=0} {return -1}
  }
  foreach h [lsort -integer [array names global_userheader]] {
    set res [write_header_to_tape $ved $do $global_userheader($h) 0]
    if {$res!=0} {return -1}
  }
  return 0
}

proc write_setup_to_dataouts {start_stop veds} {
  global global_setupdata global_init
  global global_headers_valid global_namespaces

  output "write_setup_to_dataouts $veds $start_stop"
  if {($start_stop=="start") || ($start_stop=="restart")} {
    if {$global_headers_valid!="start"} {
      create_superheader
      foreach i $global_setupdata(names) {
        create_configheader $i
      }
      create_userheader
      if {$start_stop=="start"} create_noteheader
      set global_headers_valid start
    }
  } else {
    if {$global_headers_valid!="stop"} {
      update_superheader
      set global_headers_valid stop
    }
  }

  foreach ved $veds {
    set name $global_namespaces($ved)
    if [info exists ${name}::dataout_for_config_records] {
      set dataouts [set ${name}::dataout_for_config_records]
    } elseif {[info exists ${name}::eventbuilder]
              && [set ${name}::eventbuilder]>0
              &&[info exists ${name}::dataout]} {
      set dataouts [array names ${name}::dataout]
    } else {
      set dataouts {}
    }
    foreach do $dataouts {
      output "write setup to dataout $do of ved $ved"
      set res [write_setup_to_dataout $ved $do]
      if {$res!=0} {return -1}
    }
  }
  return 0
}
