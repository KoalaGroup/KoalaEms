proc correct_setup {} {
  global global_setup

  if {[info exists global_setup(autosave)] == 0} {
    set global_setup(autosave) 0}

  if {[info exists global_setup(height)] == 0} {
    set global_setup(height) 150}

  if {[info exists global_setup(width)] == 0} {
    set global_setup(width) 1280}

  if {[info exists global_setup(show_menubar)] == 0} {
    set global_setup(show_menubar) 1}

  if {[info exists global_setup(show_scrollbar)] == 0} {
    set global_setup(show_scrollbar) 1}

  if {[info exists global_setup(show_legend)] == 0} {
    set global_setup(show_legend) 1}

  if {[info exists global_setup(x_axis_diff)] == 0} {
    set global_setup(x_axis_diff) 1200}

  if {[info exists global_setup(y_axis_max)] == 0} {
    set global_setup(y_axis_max) 200}

  if {[info exists global_setup(port)] == 0} {
    set global_setup(port) 1234}

  if {[info exists global_setup(cv_font)] == 0} {
    set global_setup(cv_font) -*-courier-medium-r-*-*-12-*-*-*-*-*-*-*}

  if {[info exists global_setup(list_font)] == 0} {
    set global_setup(list_font) -*-courier-medium-r-*-*-12-*-*-*-*-*-*-*}

  if {[info exists global_setup(autostart)] == 0} {
    set global_setup(autostart) 0}

  if {[info exists global_setup(data_autosave)] == 0} {
    set global_setup(data_autosave) 0}

  if {[info exists global_setup(data_autorestore)] == 0} {
    set global_setup(data_autorestore) 0}

  if {[info exists global_setup(data_autodir)] == 0} {
    set global_setup(data_autodir) ~/.xh}
}
