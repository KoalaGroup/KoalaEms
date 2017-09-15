proc testready {namespace} {
  if {[set ${namespace}::downcount]==0} {
    if {[set ${namespace}::ready]<0} {
      close_ved [set ${namespace}::ved]
      namespace delete $namespace
    } else {
      set ${namespace}::ready 1
      if {[lsearch -exact [set ${namespace}::domlist] 5]>=0} {
        set ${namespace}::is_em 1
      } else {
        set ${namespace}::is_em 0
      }
      init_perfvals $namespace
      ::Display::update_ved_frame [set ${namespace}::ved] $namespace
      ::Loop::add [set ${namespace}::ved]
    }
  }
}

proc gotnothing_ignore {ved args} {
  #puts "gotnothing_ignore: $ved $args"
}

proc gotnothing {ved args} {
  global dialog_nr
  set win .error_$dialog_nr
  incr dialog_nr
  tk_dialog $win Error "got no namelist from VED [$ved name]" {} 0 Dismiss
  close_ved $ved
}

proc gotdomtrigger {ved args} {
  switch [lindex $args 0] {
    zel.1 {
      set args [xdrl2s $args 1]
    }
  }
  set ::ns_${ved}::ro(trigger) $args
}

proc gotdidefinition {ved idx args} {
  set ::ns_${ved}::di_${idx}(definition) $args
}

proc gotdodefinition {ved idx args} {
  set ::ns_${ved}::do_${idx}(definition) $args
}

proc gotdomdilist {ved args} {
  set ::ns_${ved}::dataindom $args
  lappend ::ns_${ved}::tobjects dom_di
  foreach i [set ::ns_${ved}::dataindom] {
    $ved datain upload $i : gotdidefinition $ved $i ? gotnothing_ignore $ved $i
  }
  incr ::ns_${ved}::downcount -1
  testready ::ns_${ved}
}

proc gotdomdolist {ved args} {
  set ::ns_${ved}::dataoutdom $args
  lappend ::ns_${ved}::tobjects dom_do
  foreach i [set ::ns_${ved}::dataoutdom] {
    $ved dataout upload $i : gotdodefinition $ved $i ? gotnothing_ignore $ved $i
  }
  incr ::ns_${ved}::downcount -1
  testready ::ns_${ved}
}

proc gotdomlist {ved args} {
  set ::ns_${ved}::domlist $args
  # domain datain
  if {[lsearch -exact $args 5]>=0} {
    $ved namelist domain 5 : gotdomdilist $ved ? gotnothing $ved
  } else {
    lappend ::ns_${ved}::tobjects dom_di
    incr ::ns_${ved}::downcount -1
  }
  # domain dataout
  if {[lsearch -exact $args 6]>=0} {
    $ved namelist domain 6 : gotdomdolist $ved ? gotnothing $ved
  } else {
    lappend ::ns_${ved}::tobjects dom_do
    incr ::ns_${ved}::downcount -1
  }
  # domain trigger
  if {[lsearch -exact $args 3]>=0} {
    $ved trigger upload : gotdomtrigger $ved ? gotnothing_ignore $ved
  }
  testready ::ns_${ved}
}

proc gotdolist {ved args} {
  set ::ns_${ved}::dataoutobj $args
  lappend ::ns_${ved}::tobjects obj_do
  incr ::ns_${ved}::downcount -1
  testready ::ns_${ved}
}

proc gotpilist {ved args} {
  global dialog_nr
  if {[lsearch -exact $args 1]==-1} {
    set win .error_$dialog_nr
    incr dialog_nr
    tk_dialog $win Error "VED [$ved name] has no program invocation readout" {} \
      0 Dismiss
    set ::ns_${ved}::ready -1
  }
  lappend ::ns_${ved}::tobjects ro
  incr ::ns_${ved}::downcount -1
  testready ::ns_${ved}
}

proc gotnamelist {ved args} {
  global dialog_nr
  set ::ns_${ved}::namelist $args
  # domain
  if {[lsearch -exact $args 2]>=0} {
    $ved namelist domain : gotdomlist $ved ? gotnothing $ved
  } else {
    lappend ::ns_${ved}::tobjects dom_do
    lappend ::ns_${ved}::tobjects dom_di
    incr ::ns_${ved}::downcount -2
  }
  # program invocation
  if {[lsearch -exact $args 5]>=0} {
    $ved namelist pi : gotpilist $ved ? gotnothing $ved
  } else {
    set win .error_$dialog_nr
    incr dialog_nr
    tk_dialog $win Error "VED [$ved name] has no program invocation" {} \
      0 Dismiss
    lappend ::ns_${ved}::tobjects ro
    incr ::ns_${ved}::downcount -1
    set ::ns_${ved}::ready -1
  }
  # dataout
  if {[lsearch -exact $args 6]>=0} {
    $ved namelist do : gotdolist $ved ? gotnothing $ved
  } else {
    lappend ::ns_${ved}::tobjects obj_do
    incr ::ns_${ved}::downcount -1
  }
  testready ::ns_${ved}
}

proc gotident {ved args} {
  set ::ns_${ved}::ident [lindex $args 3]
  incr ::ns_${ved}::downcount -1
  testready ::ns_${ved}
}

proc get_ved_info {ved} {
  $ved namelist 0 : gotnamelist $ved ? gotnothing $ved
  $ved identify 1 : gotident $ved ? gotnothing $ved
  namespace eval ::ns_$ved {}
  set ::ns_${ved}::ved $ved
  set ::ns_${ved}::ident ""
  set ::ns_${ved}::namelist {}
  set ::ns_${ved}::domlist {}
  set ::ns_${ved}::dataindom {}
  set ::ns_${ved}::dataoutdom {}
  set ::ns_${ved}::dataoutobj {}
  set ::ns_${ved}::tobjects {}
  set ::ns_${ved}::ro(trigger) ---
  set ::ns_${ved}::ready 0
  set ::ns_${ved}::downcount 5
}
