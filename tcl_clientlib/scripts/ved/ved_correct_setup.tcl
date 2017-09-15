proc correct_setup {} {
  global global_setup

  if {[info exists global_setup(autosave)] == 0} {
    set global_setup(autosave) 0}

  if {[info exists global_setup(list_font)] == 0} {
    set global_setup(list_font) {-*-courier-medium-r-*-*-14-*-*-*-*-*-*-*}}

  if {[info exists global_setup(slist_font)] == 0} {
    set global_setup(slist_font) {-*-courier-medium-r-*-*-12-*-*-*-*-*-*-*}}

}
