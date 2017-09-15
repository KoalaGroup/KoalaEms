# $ZEL: create_headers.tcl,v 1.7 2009/04/01 16:29:58 wuestner Exp $
# copyright 2001
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc dummy_headers {} {}

namespace eval headers {

variable superheader_stem ;# first textrecord, contains runNr. and time
variable superheader  ;# updated with timestamp ...
variable masterheader ;# second textrecord, contains the VED names
variable noteheader   ;# third textrecord, containes runnotes

variable textheaders  ;# list of varnames of textheaders (stringlists)
variable fileheaders  ;# list of files

proc create_superheader_stem {} {
  variable superheader_stem
  global global_setupdata global_setup

  set superheader_stem {}
  lappend superheader_stem {Key: superheader}
  lappend superheader_stem "Facility: $global_setup(facility)"
  lappend superheader_stem "Location: $global_setup(location)"
  lappend superheader_stem "Devices: $global_setupdata(names)"

  set ::headers::superheader $superheader_stem ;# just for paranoia
  lappend ::headers::textheaders superheader ;# not superheader_stem!!
}

proc update_superheader_start {run_nr time} {
  variable superheader_stem
  variable superheader
  global clockformat

  set superheader $superheader_stem
  lappend superheader "Run: $run_nr"
  lappend superheader "Start Time: [clock format $time -format $clockformat]"
  lappend superheader "Start Time_coded: $time"
}

proc update_superheader_stop {time} {
  variable superheader
  global clockformat

  lappend superheader "Stop Time: [clock format $time -format $clockformat]"
  lappend superheader "Stop Time_coded: $time"
}

proc create_masterheader {} {
  variable masterheader

  set masterheader [mastersetup eval {
    set head {}
    lappend head {Key: masterheader}
    if [info exists RCSid] {
      lappend head [list RCSid $RCSid]
    }
    foreach i [array names setupfile] {
      lappend head [list setupfile($i) $setupfile($i)]
    }
    if [info exists export_vars] {
      lappend head [list export_vars $export_vars]
      foreach i $export_vars {
        lappend head [list $i [set $i]]
      }
    }
    return $head
  }]
  mastersetup eval unset head
  lappend ::headers::textheaders masterheader
}

proc create_vedheader {ved} {
  variable vedheader
  #output "create_vedheader $ved called" tag_orange

  set head [list {Key: configuration}]
  lappend head [list name $ved]

  set xhead [ved_setup_$ved eval {
    if [info exists RCSid] {
      lappend head [list RCSid $RCSid]
    }
    lappend head [list vedname $vedname]
    lappend head [list description $description]
    if [info exists eventbuilder] {
      lappend head [list eventbuilder $eventbuilder]
    }
    if [info exists triggermaster] {
      lappend head [list triggermaster $triggermaster]
    }
    if [array exists dataout] {
      foreach i [array names dataout] {
        lappend head [list dataout($i) $dataout($i)]
      }
    }
    if [array exists dataout_autochange] {
      foreach i [array names dataout_autochange] {
        lappend head [list dataout_autochange($i) $dataout_autochange($i)]
      }
    }
    if [array exists datain] {
      foreach i [array names datain] {
        lappend head [list datain($i) $datain($i)]
      }
    }
    if [info exists modullist] {
      lappend head [list modullist $modullist]
    }
    if [array exists isid] {
      foreach i [array names isid] {
        lappend head [list isid($i) $isid($i)]
      }
    }
    if [array exists memberlist] {
      foreach i [array names memberlist] {
        lappend head [list memberlist($i) $memberlist($i)]
      }
    }
    if [array exists readouttrigg] {
      foreach i [array names readouttrigg] {
        lappend head [list readouttrigg($i) $readouttrigg($i)]
      }
    }
    if [array exists readoutmask] {
      foreach i [array names readoutmask] {
        lappend head [list readoutmask($i) $readoutmask($i)]
      }
    }
    if [array exists readoutprio] {
      foreach i [array names readoutprio] {
        lappend head [list readoutprio($i) $readoutprio($i)]
      }
    }
    if [array exists readoutproc] {
      foreach i [array names readoutproc] {
        lappend head [list readoutproc($i) $readoutproc($i)]
      }
    }
    if [info exists trigger] {
      lappend head [list trigger $trigger]
    }
    if [array exists vars] {
      foreach i [array names vars] {
        lappend head [list vars($i) $vars($i)]
      }
    }
    if [array exists var_init] {
      foreach i [array names var_init] {
        lappend head [list var_init($i) $var_init($i)]
      }
    }
    foreach i {preinit init postinit start stop} {
      if [array exists ${i}_proclist] {
        foreach iss [array names ${i}_proclist] {
          lappend head [list ${i}_proclist($iss) [set ${i}_proclist($iss)]]
        }
      }
      if [info exists ${i}_proclist_t] {
        lappend head [list ${i}_proclist_t [set ${i}_proclist_t]]
      }
      if [array exists ${i}_command] {
        foreach iss [array names ${i}_command] {
          set name [set ${i}_command($iss)]
          lappend head [list ${i}_command($iss) $name]
          if [info exists ${i}_args($iss)] {
            lappend head [list ${i}_args($iss) [set ${i}_args($iss)]]
          }
        lappend head [list proc [list [info args $name] [info body $name]]]
        }
      }
    }
    return $head
  }]
  ved_setup_$ved eval unset head
  set vedheader($ved) [concat $head $xhead]
  lappend ::headers::textheaders vedheader($ved)
}

proc dump_textheader {head text} {
  output "$text:" tag_orange
  foreach i $head {
    output_append $i
  }
}

proc create_noteheader {} {
  variable noteheader
  set noteheader [list {Key: run_notes}]
  lappend noteheader \{[enter_notes]\}
  if {[lsearch -exact $::headers::textheaders noteheader]==-1} {
    lappend ::headers::textheaders noteheader
  }
}

# proc create_fileheaders {} {
#   global global_setupdata global_verbose
# 
#   #output "create_fileheaders called" tag_orange
#   set sourcelist {}
#   set s [mastersetup eval array names sourcelist]
#   foreach i $s {
#     if {[lsearch -exact $sourcelist $i]==-1} {lappend sourcelist $i}
#   }
#   foreach ved $global_setupdata(names) {
#     set s [ved_setup_$ved eval array names sourcelist]
#     foreach i $s {
#       if {[lsearch -exact $sourcelist $i]==-1} {lappend sourcelist $i}
#     }
#   }
# 
#   foreach name [lsort -dictionary $sourcelist] {
#     lappend ::headers::fileheaders $name
#   }
#   if {$global_verbose} {
#     output "sourced files:" tag_blue
#     foreach name $::headers::fileheaders {
#       output_append "$name" tag_blue
#     }
#   }
# }

proc create_fileheaders {} {
    global global_setupdata global_verbose

    set list [mastersetup eval array names sourcelist]

    foreach ved $global_setupdata(names) {
        set s [ved_setup_$ved eval array names sourcelist]
        set list [concat $list $s]
    }

    set again 1
    while {$again} {
        set again 0
        set nlist {}
        foreach n $list {
            lappend nlist [file normalize $n]
        }

        set slist [lsort -dictionary -unique $nlist]

        set list {}
        foreach n $slist {
            if {[file isdirectory $n]} {
                set again 1
                set dlist [glob -nocomplain -join $n *]
                set list [concat $list $dlist]
            } else {
                lappend list $n
            }
        }

    }

    set nlist {}
    foreach n $list {
        lappend nlist [file normalize $n]
    }

    set ::headers::fileheaders [lsort -dictionary -unique $nlist]

    if {$global_verbose} {
        output "sourced files:" tag_blue
        foreach name $::headers::fileheaders {
            output_append "$name" tag_blue
        }
    }
}

proc create_headers {} {
  global global_setupdata

  set ::headers::textheaders {}
  set ::headers::fileheaders {}

  create_superheader_stem
  #dump_textheader $::headers::superheader Superheader
  create_masterheader
  #dump_textheader $::headers::masterheader Masterheader
  foreach i $global_setupdata(names) {
    create_vedheader $i
    #dump_textheader $::headers::vedheader($i) VEDheader($i)
  }
  create_fileheaders
}

}
