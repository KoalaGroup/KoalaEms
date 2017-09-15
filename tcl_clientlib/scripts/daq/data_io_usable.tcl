# $ZEL: data_io_usable.tcl,v 1.3 2009/03/29 20:09:35 wuestner Exp $
# copyright 1999
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
proc data_in_outs_usable {level} {
  global global_setupdata global_verbose
  global global_interps

# check dataouts if ved.level >= level
# check datains if ved.level > level
# level 0: cc, ccm
# level 1: em (ser)
# level 2: additional eventmanager (eb)

  set do_veds {}
  if {$level==0} {
    set do_veds $global_setupdata(all_veds)
  } else {
    foreach ved $global_setupdata(em_ved) {
      set name $global_interps($ved)
      if {[$name eval set eventbuilder]>=$level} {lappend do_veds $ved}
    }
  }

  set di_veds {}
  foreach ved $global_setupdata(em_ved) {
    set name $global_interps($ved)
    if {[$name eval set eventbuilder]>$level} {lappend di_veds $ved}
  }

  foreach ved $do_veds {
    set name $global_interps($ved)
    if [$name eval info exists dataout] {
      foreach do [$name eval array names dataout] {
        #output "check dataout $do of ved $ved"
        if [catch {set status [$ved dataout status $do]} mist] {
          output "Dataout status of dataout $do of VED $ved: $mist" tag_red
          return -1
        }
        set stat [lindex $status 1]
        set err [lindex $status 0]
        #output "stat: $stat err: $err"
        if {$stat==3} {
          output "Status of dataout $do of VED $ved (stat: $stat err: $err) is not acceptable." tag_red
          return -1
        }
      }
    }
  }

  foreach ved $di_veds {
    #output "check datains of ved $ved"
    if [catch {set status [$ved readout status 1 2]} mist] {
      output "Readoutstatus of VED $ved: $mist" tag_red
      return -1
    }
    set stat [lindex $status 0]
    #output "status: $stat"
    set fehl 0
    switch $stat {
      error {set fehl 1}
      no_more_data {set fehl 0}
      stopped {set fehl 1}
      inactive {set fehl 1}
      running {set fehl 0}
      default {
        output "unknown ro-status $stat from VED $ved" tag_red
        return -1
      }
    }
    if {$fehl!=0} {
      output "Readoutstatus of VED $ved ($stat) is not acceptable." tag_red
      return -1
    }
  }

  return 0  
}
