#
# global vars in this file:
#
# global_setup(commu_autoconnect)
#
#
#

proc autostart {} {
  global global_setup
  if [info exists global_setup(offline)] {
    if $global_setup(offline) return
  }
  set raus 0
  if [info exists global_setup(commu_autoconnect)] {
    if $global_setup(commu_autoconnect) {
      output "try connect with commu..."
      update idletasks
      if {[connect_commu]==-1} {
        output_append "without success"
        set raus 1
      } else {
        output_append "successfull"
      }
      update idletasks
    }
  }
  if $raus return
  if [info exists global_setup(veds)] {
    # muss kopiert werden, weil open_ved global_setup(veds) benutzt.
    set veds $global_setup(veds)
    unset global_setup(veds)
    foreach ved $veds {
      # global_setup($ved) wird von open_ved ueberschrieben
      set l $global_setup($ved)
      output "try open [lindex $l 0] as [lindex $l 1]..."
      update idletasks
      if {[open_ved [lindex $l 0] [lindex $l 1]]==-1} {
        output_append "without success"
      } else {
        output_append "successfull"
      }
    update_status
    update idletasks
    }
  }
}
