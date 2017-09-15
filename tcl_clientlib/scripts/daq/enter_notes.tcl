# $ZEL: enter_notes.tcl,v 1.5 2003/02/04 19:27:52 wuestner Exp $
# copyright: 1998, 2001
# Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc notes_ok {} {
  set ::headers::notes_text [.notes.t get 1.0 end]
  set ::headers::notes_entered 1
}

proc notes_include {} {
  if [info exists ::headers::notes_text] {
    .notes.t insert end $::headers::notes_text
  }
}

proc notes_no {} {
  set ::headers::notes_text {}
  set ::headers::notes_entered 1
}

proc enter_notes {} {

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
  set ::headers::notes_entered 0
  vwait ::headers::notes_entered
  destroy .notes
  return $::headers::notes_text
}
