#
# $Id: varframe.tcl,v 1.1 2000/07/15 21:34:40 wuestner Exp $
# © 1999 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# varframe::varname
# varframe::vartype
# varframe::varval
# varframe::idxs      valid indices for varname ... there is a bug in unset ...
#
#  global:
# none
#
#  varframe:
# idxs                list of all valid variable-indices
# idx                 next unused variable-index
# w                   pathname of containing frame
# var_<idx>()         array for variable <idx>
#   name
#   type
#   otype             original type (needed for save/restore)
#   val
#   autoread
#   autowrite
#   rerror
#   werror
#   rerrorw
#   werrorw
#   
#   

proc dummy_varframe {} {}

namespace eval varframe {

# protection against auto_reset and subsequent autoload
if {![info exists varframe::idx]} {
  variable idx 0
}

proc do_it {} {
  variable idxs
  global global_color

  output "must write:"
  foreach i [lsort -integer $idxs] {
    variable var_$i
    if [set var_${i}(autowrite)] {
      output_append $i
      set list {}
      lappend list [set var_${i}(name)]
      lappend list [set var_${i}(otype)]
      set write_arr($i) $list
    }
  }  
  output "must read:"
  foreach i [lsort -integer $idxs] {
    variable var_$i
    if [set var_${i}(autoread)] {
      output_append $i
      set list {}
      lappend list [set var_${i}(name)]
      lappend list [set var_${i}(otype)]
      set read_arr($i) $list
    }
  }
  set res [wincc_vars write_arr read_arr]
  if {$res!=0} return
  foreach i [array names write_arr] {
    set var_${i}(werror) $write_arr($i)
    if {$write_arr($i)!=0} {
      [set var_${idx}(werrorw)] configure -bg red
    } else {
      [set var_${idx}(werrorw)] configure -bg $global_color(backcolor)
    }
  }
  foreach i [array names write_arr] {
    set err [lindex $read_arr($i) 0]
    set var_${i}(val) [lindex $read_arr($i) 1]
    set var_${i}(rerror) $read_arr($i)
    if {$err!=0} {
      [set var_${idx}(rerrorw)] configure -bg red
    } else {
      [set var_${idx}(rerrorw)] configure -bg $global_color(backcolor)
    }
  }
}

proc delete_var {} {
  variable w
  variable idxs

  set win [selection own]
  if {$win==""} { bell; return }

  set n [regsub ".vframe.varframe.(name_|type_|value_)" $win {} x]
  if {$n<1} { bell; return }

  set iii [lsearch -exact $idxs $x]
  if {$iii<0} {
    output "internal error: idx $x does not exist" tag_blue
    bell
    return
    }
  set idxs [lreplace $idxs $iii $iii]
  variable var_$idx

  set info [grid info $win]
  output "info: $info" -tag_orange
  unset var_$xname

  destroy $w.name_$x
  destroy $w.type_$x
  destroy $w.value_$x
  destroy $w.read_$x
  destroy $w.write_$x
  destroy $w.autowrite_$x
  destroy $w.autoread_$x
  rearrange
}

proc write_var {idx} {
  variable var_$idx
  global global_color

  set wincc_err 0
  set var_err 0
  write_wincc_var [set var_${idx}(type)] [set var_${idx}(name)] \
      [set var_${idx}(val)] wincc_err var_err]
  if {$wincc_err!=0} {
    set var_${idx}(werror) ???
    [set var_${idx}(werrorw)] configure -bg yellow
    return
  }
  set var_${idx}(werror) $var_err
  if {[set var_${idx}(werror)]!=0} {
    [set var_${idx}(werrorw)] configure -bg red
  } else {
    [set var_${idx}(werrorw)] configure -bg $global_color(backcolor)
  }
}

proc read_var {idx} {
  variable var_$idx
  global global_color

  set wincc_err 0
  set var_err 0
  set var_${idx}(val) [read_wincc_var [set var_${idx}(type)] \
      [set var_${idx}(name)] wincc_err var_err]
  if {$wincc_err!=0} {
    set var_${idx}(rerror) ???
    [set var_${idx}(rerrorw)] configure -bg yellow
    return
  }
  set var_${idx}(rerror) $var_err
  if {[set var_${idx}(rerror)]!=0} {
    [set var_${idx}(rerrorw)] configure -bg red
  } else {
    [set var_${idx}(rerrorw)] configure -bg $global_color(backcolor)
  }
}

proc create_head {} {
  variable w

  frame $w.bar -width 2 -relief flat -bg black
  label $w.laut -text automatic
  label $w.lres -text Errorcode
  label $w.lname -text Name
  label $w.ltype -text Type
  label $w.lvalue -text Value
  label $w.lread -text read
  label $w.lwrite -text write
  label $w.lresr -text (read)
  label $w.lresw -text (write)
  label $w.lautoread -text read
  label $w.lautowrite -text write
}

proc create_varline {name type} {
  variable w
  variable idx
  variable idxs
  variable var_$idx

  lappend idxs $idx
  set var_${idx}(name) $name
  if {$name==""} {
    entry $w.name_$idx -textvariable [namespace current]::var_${idx}(name) \
        -relief sunken
  } else {
    entry $w.name_$idx -textvariable [namespace current]::var_${idx}(name) \
        -relief flat -state disabled
  }
  set var_${idx}(otype) $type
  if {$type==""} {
    set var_${idx}(type) char
    menubutton $w.type_$idx -textvariable [namespace current]::var_${idx}(type) \
        -indicatoron 1 -menu $w.type_$idx.menu -width 6\
        -relief raised -bd 2 -highlightthickness 0 -anchor c -pady -1p
    menu $w.type_$idx.menu -tearoff 0
    foreach i {bit byte word dword sbyte sword char double float raw} {
      $w.type_$idx.menu add command -label $i \
          -command [namespace code "set var_${idx}(type) $i"]
    }
  } else {
    set var_${idx}(type) $type
    entry $w.type_$idx -textvariable [namespace current]::var_${idx}(type) \
        -relief flat -state disabled -width 8
  }
  entry $w.value_$idx -textvariable [namespace current]::var_${idx}(val) \
      -relief sunken
  button $w.read_$idx -command [namespace code "read_var $idx"] -pady -2m \
      -text <<<<
  button $w.write_$idx -command [namespace code "write_var $idx"] -pady -2m \
      -text >>>>
  entry $w.resr_$idx -textvariable [namespace current]::var_${idx}(rerror) \
      -relief flat -state disabled -width 8 -justify right
  entry $w.resw_$idx -textvariable [namespace current]::var_${idx}(werror) \
      -relief flat -state disabled -width 8 -justify right
  checkbutton $w.autowrite_$idx -variable \
      [namespace current]::var_${idx}(autowrite) -selectcolor black
  checkbutton $w.autoread_$idx -variable \
      [namespace current]::var_${idx}(autoread) -selectcolor black
  set var_${idx}(val) ""
  set var_${idx}(autoread) 0
  set var_${idx}(autowrite) 0
  set var_${idx}(rerror) ""
  set var_${idx}(werror) ""
  set var_${idx}(rerrorw) $w.resr_$idx
  set var_${idx}(werrorw) $w.resw_$idx
  incr idx
}

proc rearrange {} {
  variable w
  variable idxs

  foreach slave [grid slaves $w] {
    grid forget $slave
  }
  grid  x x x x x $w.lres - $w.bar  $w.laut -
  grid configure $w.bar -sticky ns
  grid $w.lname $w.ltype $w.lvalue $w.lread $w.lwrite $w.lresr $w.lresw \
      ^ $w.lautoread $w.lautowrite

  foreach idx [lsort -integer $idxs] {
    grid $w.name_$idx $w.type_$idx $w.value_$idx $w.read_$idx $w.write_$idx \
      $w.resr_$idx $w.resw_$idx ^ $w.autoread_$idx $w.autowrite_$idx -padx 5
  }
  grid x x x x x x $w.dummy ^ $w.actbutton -
}

proc create_actbutton {} {
  variable w

  button $w.actbutton -text "do it" -command [namespace code do_it]
  frame $w.dummy
}

proc create {mainframe} {
  variable w
  variable idxs
  global global_setup_wccvars

  set idxs {}
  set w [frame $mainframe -relief flat -borderwidth 2]
  create_head
  create_actbutton
  foreach i [lsort -integer [array names global_setup_wccvars]] {
    create_varline [lindex $global_setup_wccvars($i) 0] \
        [lindex $global_setup_wccvars($i) 1]
  }
  rearrange
  pack $w -side top -fill both
}

proc mk_setup {} {
  variable idxs
  global global_setup_extra
  global global_setup_wccvars

  ladd global_setup_extra global_setup_wccvars
  if [info exists global_setup_wccvars] {
    unset global_setup_wccvars
  }
  set n 0
  foreach i [lsort -integer $idxs] {
    variable var_$i
    set list {}
    lappend list [set var_${i}(name)]
    lappend list [set var_${i}(otype)]
    set global_setup_wccvars($n) $list
    incr n
  }
}

}
