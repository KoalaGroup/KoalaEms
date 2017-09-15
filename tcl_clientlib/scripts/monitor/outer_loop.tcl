# $Id: outer_loop.tcl,v 1.2 1998/09/14 12:10:14 wuestner Exp $
# © 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# global_after(outer_loop)
# global_setup(interval)
# global_setup(veds)
# global_status($name)
# global_count($name)
# global_time
# 
#-----------------------------------------------------------------------------#
proc outer_loop {} {
  global global_after
  global global_setup
  global global_status
  global global_count
  global global_time

  putm ""
  if {![ems_connected]} {
    connect_commu
  }

  if {![ems_connected]} {
    .bar configure -background #f00
    set global_after(outer_loop) [after 60000 outer_loop]
    return
  }
  .bar configure -background #00f

  foreach name $global_setup(veds) {
    set ved_open($name) 0
  }

  foreach ved [ems_openveds] {
    set ved_open([$ved name]) 1
  }

  foreach name $global_setup(veds) {
    if {!$ved_open($name)} {
      putm "open VED $name"
      if [catch {ems_open $name} mist] {
        putm $mist
        .disp.name_$name configure -background #f00
      } else {
        .disp.name_$name configure -background #d9d9d9
      }
    }
  }

  foreach ved [ems_openveds] {
    set name [$ved name]
    if [catch {
      set stat [$ved readout status]
      set global_status($name) [lindex $stat 0]
      set global_count($name) [lindex $stat 1]
      .disp.name_$name configure -background #d9d9d9
      switch $global_status($name) {
        error        {.disp.stat_$name configure -background #f00}
        no_more_data {.disp.stat_$name configure -background #0f0}
        stopped      {.disp.stat_$name configure -background #ff0}
        inactive     {.disp.stat_$name configure -background #af0}
        running      {.disp.stat_$name configure -background #d9d9d9}
      }
    } mist ] {
      rename $ved {}
      .disp.name_$name configure -background #f00
    }
  }
  set global_time [clock format [clock seconds] -format {%b %d %H:%M:%S}]
  set global_after(outer_loop) [after $global_setup(interval) outer_loop]
}
#-----------------------------------------------------------------------------#
proc restart_outer_loop {} {
  global global_after

  foreach i [array names global_after] {
    after cancel $global_after($i)
  }
  set global_after(outer_loop) [after 100 outer_loop]
}
#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
