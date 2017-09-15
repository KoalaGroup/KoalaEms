namespace eval ::Display {}

proc ::Display::update_dicc_frame {self namespace} {
  foreach i [winfo children $self] {
    destroy $i
  }
  label $self.l -text Readout -anchor w
  frame $self.t
  frame $self.a
  label $self.a.tl -text Trigger -anchor w
  entry $self.a.te -state disabled -textvariable ${namespace}::ro(trigger)
  label $self.a.l_1 -text "Event Rate" -anchor w
  entry $self.a.e_1 -state disabled -justify right -textvariable ${namespace}::ro(revents)
  label $self.a.l_2 -text Suspensions -anchor w
  entry $self.a.e_2 -state disabled -justify right -textvariable ${namespace}::ro(susp)
  entry $self.a.e_2r -state disabled -justify right -textvariable ${namespace}::ro(rsusp)
  label $self.a.l_3 -text "Event Size" -anchor w
  entry $self.a.e_3 -state disabled -justify right -textvariable ${namespace}::ro(evsize)
  label $self.a.l_4 -text Copies -anchor w
  entry $self.a.e_4 -state disabled -justify right -textvariable ${namespace}::ro(copies)
  entry $self.a.e_4r -state disabled -justify right -textvariable ${namespace}::ro(rcopies)
  grid configure $self.a.tl $self.a.te - -sticky ew
  grid configure $self.a.l_1 $self.a.e_1 -sticky ew
  grid configure $self.a.l_2 $self.a.e_2 $self.a.e_2r -sticky ew
  grid configure $self.a.l_3 $self.a.e_3 -sticky ew
  grid configure $self.a.l_4 $self.a.e_4 $self.a.e_4r -sticky ew
  pack $self.l $self.t $self.a -side top -fill x -expand 1
}

proc ::Display::update_diem_frame {self namespace} {
  foreach i [winfo children $self] {
    destroy $i
  }
  if {[llength [set ${namespace}::dataindom]]==0} {
    label $self.l -text "no Datains" -anchor w
    pack $self.l -side top -fill x -expand 1
  } else {
    foreach i [set ${namespace}::dataindom] {
      set frame $self.a_$i
      frame $frame -relief raised
      label $frame.l -text "Datain $i" -anchor w
      frame $frame.a
      label $frame.a.tl -text Definition -anchor w
      entry $frame.a.te -state disabled -textvariable ${namespace}::di_${i}(definition)
      label $frame.a.l_1 -text Status -anchor w
      entry $frame.a.e_1 -state disabled -justify right -textvariable ${namespace}::di_${i}(status)
      label $frame.a.l_2 -text Suspensions -anchor w
      entry $frame.a.e_2 -state disabled -justify right -textvariable ${namespace}::di_${i}(susp)
      entry $frame.a.e_2r -state disabled -justify right -textvariable ${namespace}::di_${i}(rsusp)
      label $frame.a.l_3 -text Cluster -anchor w
      entry $frame.a.e_3 -state disabled -justify right -textvariable ${namespace}::di_${i}(clust)
      entry $frame.a.e_3r -state disabled -justify right -textvariable ${namespace}::di_${i}(rclust)
#     label $frame.a.l_4 -text Events -anchor w
#     entry $frame.a.e_4 -state disabled -justify right -textvariable ${namespace}::di_${i}(events)
#     entry $frame.a.e_4r -state disabled -justify right -textvariable ${namespace}::di_${i}(revents)
      label $frame.a.l_5 -text Bytes -anchor w
      entry $frame.a.e_5 -state disabled -justify right -textvariable ${namespace}::di_${i}(kbytes)
      entry $frame.a.e_5r -state disabled -justify right -textvariable ${namespace}::di_${i}(rkbytes)
      grid configure $frame.a.tl $frame.a.te - -sticky ew
      grid configure $frame.a.l_1 $frame.a.e_1 -sticky ew
      grid configure $frame.a.l_2 $frame.a.e_2 $frame.a.e_2r -sticky ew
      grid configure $frame.a.l_3 $frame.a.e_3 $frame.a.e_3r -sticky ew
#     grid configure $frame.a.l_4 $frame.a.e_4 $frame.a.e_4r -sticky ew
      grid configure $frame.a.l_5 $frame.a.e_5 $frame.a.e_5r -sticky ew
      pack $frame.l $frame.a -side top -fill x -expand 1
      pack $frame -side top -fill x -expand 1
    }
  }
}

proc ::Display::update_do_frame {self namespace} {
  foreach i [winfo children $self] {
    destroy $i
  }
  if {[llength [set ${namespace}::dataoutdom]]==0} {
    label $self.l -text "no Dataouts" -anchor w
    pack $self.l -side top -fill x -expand 1
  } else {
    foreach i [set ${namespace}::dataoutdom] {
      set frame $self.a_$i
      frame $frame -relief raised
      label $frame.l -text "Dataout $i" -anchor w
      frame $frame.a
      label $frame.a.tl -text Definition -anchor w
      entry $frame.a.te -state disabled -textvariable ${namespace}::do_${i}(definition)
      label $frame.a.l_1 -text Status -anchor w
      entry $frame.a.e_1 -state disabled -justify right -textvariable ${namespace}::do_${i}(status) 
      label $frame.a.l_2 -text Switch -anchor w
      entry $frame.a.e_2 -state disabled -justify right -textvariable ${namespace}::do_${i}(switch)
      label $frame.a.l_3 -text Cluster -anchor w
      entry $frame.a.e_3 -state disabled -justify right -textvariable ${namespace}::do_${i}(clust)
      entry $frame.a.e_3r -state disabled -justify right -textvariable ${namespace}::do_${i}(rclust)
      label $frame.a.l_4 -text Bytes -anchor w
      entry $frame.a.e_4 -state disabled -justify right -textvariable ${namespace}::do_${i}(kbytes)
      entry $frame.a.e_4r -state disabled -justify right -textvariable ${namespace}::do_${i}(rkbytes)
      label $frame.a.l_5 -text Suspensions -anchor w
      entry $frame.a.e_5 -state disabled -justify right -textvariable ${namespace}::do_${i}(susp)
      entry $frame.a.e_5r -state disabled -justify right -textvariable ${namespace}::do_${i}(rsusp)
      label $frame.a.l_6 -text Buffer -anchor w
      entry $frame.a.e_6 -state disabled -justify right -textvariable ${namespace}::do_${i}(buff)
      entry $frame.a.e_6r -state disabled -justify right -textvariable ${namespace}::do_${i}(rbuff)
      grid configure $frame.a.tl $frame.a.te - -sticky ew
      grid configure $frame.a.l_1 $frame.a.e_1 -sticky ew
      grid configure $frame.a.l_2 $frame.a.e_2 -sticky ew
      grid configure $frame.a.l_3 $frame.a.e_3 $frame.a.e_3r -sticky ew
      grid configure $frame.a.l_4 $frame.a.e_4 $frame.a.e_4r -sticky ew
      grid configure $frame.a.l_5 $frame.a.e_5 $frame.a.e_5r -sticky ew
      grid configure $frame.a.l_6 $frame.a.e_6 $frame.a.e_6r -sticky ew
      pack $frame.l $frame.a -side top -fill x -expand 1
      pack $frame -side top -fill x -expand 1
    }
  }
}

proc ::Display::update_ved_frame {ved namespace} {
  variable static_disp
  set self [set ${namespace}::frame]
  regsub ^ved_ "$ved" {} label
  append label "; [set ${namespace}::ident]"
  if [set ${namespace}::is_em] {
    append label " (EventBuilder)"
    update_diem_frame $self.di $namespace
  } else {
    update_dicc_frame $self.di $namespace
  }
  $self.h.l configure -text $label
  update_do_frame $self.do $namespace
  update idletasks
  scale_canvas
}

proc ::Display::create_ved_frame {ved self namespace} {
  variable static_disp
  frame $self -relief ridge -borderwidth 4
  frame $self.h -relief raised -borderwidth 2
  regsub ^ved_ $ved {} label
  label $self.h.l -text $label -font "-*-times-bold-r-*-*-20-*-*-*-*-*-*-*"
  frame $self.h.g
  label $self.h.g.l_1 -text status -anchor w
  entry $self.h.g.e_1 -state disabled -justify right -textvariable ${namespace}::ro(status)
  label $self.h.g.l_2 -text events -anchor w
  entry $self.h.g.e_2 -state disabled -justify right -textvariable ${namespace}::ro(events)
  grid configure $self.h.g.l_1 $self.h.g.e_1 -sticky ew
  grid configure $self.h.g.l_2 $self.h.g.e_2 -sticky ew
  pack $self.h.l $self.h.g -side top -fill both -expand 1

  frame $self.di -relief raised -borderwidth 2
  label $self.di.l -text Datain
  frame $self.do -relief raised -borderwidth 2
  label $self.do.l -text Dataout
  pack $self.di.l -side top -fill x -expand 1
  pack $self.do.l -side top -fill x -expand 1

  pack $self.h -side top -fill x -expand 1
  pack $self.di $self.do -side left -fill x -expand 1 -anchor n
}

proc ::Display::add_ved {ved namespace} {
  variable static_disp
  set self $static_disp(fr).$ved
  set ${namespace}::frame $self
  create_ved_frame $ved $self $namespace
  grid configure $self -sticky we
  update idletasks
  scale_canvas
}

proc ::Display::delete_ved {ved} {
  variable static_disp
  destroy $static_disp(fr).$ved
  update idletasks
  scale_canvas
}

proc ::Display::dead_ved {ved} {
  variable static_disp
  $static_disp(fr).$ved.h.l configure -foreground gray50
}

proc ::Display::scale_canvas {} {
  variable static_disp
  
  set width [winfo reqwidth $static_disp(fr)]
  set height [winfo reqheight $static_disp(fr)]
  $static_disp(cv) configure -scrollregion "0 0 $width $height"
  set screenheight [winfo screenheight .]
  if {$height>$screenheight-100} {
    set height [expr $screenheight-100]
  }
  $static_disp(cv) configure -width $width -height $height
}

proc ::Display::create_display {frame} {
  variable static_disp

  set static_disp(cv) [canvas $frame.cv]
  set static_disp(sb) [scrollbar $frame.sb\
      -command "$static_disp(cv) yview"]
  $static_disp(cv) configure -yscrollcommand "$static_disp(sb) set"
  set static_disp(fr) [frame $frame.cv.frame -relief raised\
      -borderwidth 1]
  $frame.cv create window 0 0 -window $frame.cv.frame -anchor nw

  pack $frame.cv -side left -expand 1 -fill both
  pack $frame.sb -side left -fill y
  grid columnconfigure $frame.cv {0 1}  -weight 1
}
#}
