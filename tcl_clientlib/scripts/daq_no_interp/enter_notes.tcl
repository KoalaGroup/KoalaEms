# $Id: enter_notes.tcl,v 1.3 1999/04/09 12:32:33 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# notes_entered
# notes_text
#

proc notes_ok {} {
  global notes_entered notes_text
  set notes_text [.notes.t get 1.0 end]
  set notes_entered 1
}

proc notes_include {} {
  global notes_text
  if [info exists notes_text] {
    .notes.t insert end $notes_text
  }
}

proc notes_no {} {
  global notes_entered notes_text
  set notes_text {}
  set notes_entered 1
}

proc enter_notes {} {
  global notes_entered notes_text

  if [winfo exists .notes] {destroy .notes}
  toplevel .notes
  wm withdraw .notes
  wm title .notes {Notes for next Run}
  label .notes.l -text "Please enter some notes for the next run:"
  text .notes.t -wrap word
  frame .notes.bf
  button .notes.bf.o -text OK -command notes_ok
  button .notes.bf.i -text "Include old notes" -command notes_include
  # button .notes.bf.w -text "Start without notes" -command notes_no
  pack .notes.bf.o -side left -expand 1 -fill x
  pack .notes.bf.i -side left -expand 1 -fill x
  # pack .notes.bf.w -side left -expand 1 -fill x
  pack .notes.l -side top -expand 1 -fill x
  pack .notes.t -side top -expand 1 -fill both
  pack .notes.bf -side top -expand 1 -fill x
  wm deiconify .notes
  set notes_entered 0
  vwait notes_entered
  destroy .notes
  return $notes_text
}
