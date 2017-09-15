# $Id: outer_loop.tcl,v 1.2 1998/10/07 10:37:20 wuestner Exp $
# © 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# global_after(outer_loop)
# global_setup(interval)
# global_setup(veds)
# 
#-----------------------------------------------------------------------------#
proc outer_loop {} {
  global global_after
  global global_setup

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

  ems_confmode asynchron
  foreach name $global_setup(veds) {
    if {!$ved_open($name)} {
      putm "open VED $name"
      if [catch {ems_open $name} mist] {
        putm $mist
        .disp.$name.name configure -background #f00
      } else {
        .disp.$name.name configure -background #d9d9d9
      }
    }
  }

  foreach ved [ems_openveds] {
    set name [$ved name]
    global $name

    $ved readout status : got_ro_stat $ved ? got_ro_stat_err $ved
    $ved namelist do : got_do_list $ved ? got_do_list_err $ved
    $ved command {SelectStat {1}} : got_sel_stat $ved ? got_sel_stat_err $ved
    set ::${name}::ved $name
    set ::${name}::ved_ $ved
    namespace eval ::${name} {
      if {$tested_GetSyncStatist==0} {
        set has_GetSyncStatist [test_caplist $ved_ GetSyncStatist.1]
      }
      if {$has_GetSyncStatist} {
        $ved_ command {GetSyncStatist {3 3 -1 0}} : got_sync_statist $ved ? got_sync_statist_err $name
      }
    }
  }
  set global_after(outer_loop) [after $global_setup(interval) outer_loop]
}
#-----------------------------------------------------------------------------#
proc restart_outer_loop {} {
  global global_after

  foreach i [array names global_after] {
    after cancel $global_after($i)
  }
  catch {ems_disconnect}
  set global_after(outer_loop) [after 100 outer_loop]
}
#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
