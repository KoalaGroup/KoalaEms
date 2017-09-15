#$Id: set_options.tcl,v 1.3 1998/06/29 18:02:14 wuestner Exp $
proc set_font_options {} {
  global global_setup
  option add *Menubutton.font -*-Helvetica-bold-r-*-*-12-*-*-*-*-*-*-* startupFile
  if [info exists global_setup(Menubutton.font)] {
    option add *Menubutton.font $global_setup(Menubutton.font) 70
  }
  option add *Menu.font -*-Helvetica-bold-r-*-*-12-*-*-*-*-*-*-* startupFile
  if [info exists global_setup(Menu.font)] {
    option add *Menu.font $global_setup(Menu.font) 70
  }
  option add *Button.font -*-Helvetica-bold-r-*-*-12-*-*-*-*-*-*-* startupFile
  if [info exists global_setup(Button.font)] {
    option add *Button.font $global_setup(Button.font) 70
  }
  option add *Radiobutton.font -*-Helvetica-bold-r-*-*-12-*-*-*-*-*-*-* startupFile
  if [info exists global_setup(Radiobutton.font)] {
    option add *Radiobutton.font $global_setup(Radiobutton.font) 70
  }
  option add *Checkbutton.font -*-Helvetica-bold-r-*-*-12-*-*-*-*-*-*-* startupFile
  if [info exists global_setup(Checkbutton.font)] {
    option add *Checkbutton.font $global_setup(Checkbutton.font) 70
  }
  option add *Label.font -*-Helvetica-bold-r-*-*-12-*-*-*-*-*-*-* startupFile
  if [info exists global_setup(Label.font)] {
    option add *Label.font $global_setup(Label.font) 70
  }
  option add *Entry.font -*-Helvetica-medium-r-*-*-12-*-*-*-*-*-*-* startupFile
  if [info exists global_setup(Entry.font)] {
    option add *Entry.font $global_setup(Entry.font) 70
  }
}

proc set_options {} {
  set_font_options
}
