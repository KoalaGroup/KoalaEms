#
# $Id: varframe.tcl,v 1.1 2000/07/15 21:33:35 wuestner Exp $
# © 1999 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# varframe::varname
# varframe::varsize
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
#   size
#   addr
#   hval
#   dval
#

proc dummy_varframe {} {}

namespace eval varframe {

variable font -*-courier-medium-r-*-*-12-*-*-*-*-*-*-*

# protection against auto_reset and subsequent autoload
if {![info exists varframe::idx]} {
  variable idx 0
}

proc delete_var {} {
  variable w
  variable idxs

  set win [selection own]
  if {$win==""} { bell; return }

  #output "selected win: $win"
  #output "idxs: $idxs"
  set n [regsub ".vframe.varframe.(name_|addr_|size_|value_)" $win {} x]
  if {$n<1} { bell; return }
  #output "n: $n x: $x"

  set iii [lsearch -exact $idxs $x]
  #output "iii: $iii"
  if {$iii<0} {
    output "internal error: idx $x does not exist" tag_blue
    bell
    return
    }
  set idxs [lreplace $idxs $iii $iii]
  #variable var_$idx

  set info [grid info $win]
  #output "info: $info" -tag_orange
  variable var_$x
  unset var_$x

  destroy $w.name_$x
  destroy $w.addr_$x
  destroy $w.size_$x
  destroy $w.hval_$x
  destroy $w.dval_$x
  destroy $w.read_$x
  destroy $w.write_$x
  rearrange
}

proc write_var {idx} {
  global global_ved
  variable var_$idx

  if {$global_ved==""} {
    if [connect_to_server] return
  }

  set name [set var_${idx}(name)]
  set addr [set var_${idx}(addr)]
  set val  [set var_${idx}(dval)]
  set size [set var_${idx}(size)]
  output "write addr $addr, value $val, size $size"
  if [catch {
    $global_ved command1 VMEwrite $addr [expr $size/8] 1 $val
  } mist] {
    output "VMEwrite: $mist" tag_red
  }
}

proc read_var {idx} {
  global global_ved
  variable var_$idx

  if {$global_ved==""} {
    if [connect_to_server] return
  }

  set name [set var_${idx}(name)]
  set addr [set var_${idx}(addr)]
  set val  [set var_${idx}(dval)]
  set size [set var_${idx}(size)]
  output "read addr $addr, size $size"
  if [catch {
    set res [$global_ved command1 VMEread $addr [expr $size/8] 1]
  } mist] {
    output "VMEread: $mist" tag_red
    set var_${idx}(dval) ""
    set var_${idx}(hval) ""
  } else {
    switch $size {
       8 {
        set res [expr $res&0xff]
        set var_${idx}(hval) [format {0x%02X} $res]
        }
      16 {
        set res [expr $res&0xffff]
        set var_${idx}(hval) [format {0x%04X} $res]
        }
      32 {
        set var_${idx}(hval) [format {0x%08X} $res]
        }
      default {output "VMEread: idiotic size $size" tag_red}
    }
    set var_${idx}(dval) $res
  }
}

proc create_head {} {
  variable w

  label $w.lname -text Name
  label $w.laddr -text Address
  label $w.lsize -text Size
  label $w.lvalue -text Value
  label $w.lvhex -text hex
  label $w.lvdec -text dec
  label $w.lread -text read
  label $w.lwrite -text write
}

proc h2d {var} {
  variable $var
  set hval [set ${var}(hval)]
  if {$hval==""} return
  if {[string range $hval 0 1]!="0x" && [string range $hval 0 1]!="0X"} {set hval "0x$hval"}
  # conversion to dec; format does not work for hval>0x7fffffff
  set ${var}(dval) [expr $hval]
  set ${var}(hval) $hval
}

proc d2h {var} {
  variable $var
  set dval [set ${var}(dval)]
  if {$dval==""} return
  switch [set ${var}(size)] {
     8 {set ${var}(hval) [format {0x%02X} $dval]}
    16 {set ${var}(hval) [format {0x%04X} $dval]}
    32 {
      if {$dval>0x7fffffff} {
        set ${var}(hval) [format {0x%08X} [expr $dval-0x100000000]]
      } else {
        set ${var}(hval) [format {0x%08X} $dval]
      }
    }
  }
}

proc create_varline {name addr size} {
  variable w
  variable idx
  variable idxs
  variable font
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
  set var_${idx}(addr) $addr
  if {$addr==0 || $addr==""} {
  entry $w.addr_$idx -textvariable [namespace current]::var_${idx}(addr) \
      -relief sunken -width 10 -justify right -font $font
  } else {
  entry $w.addr_$idx -textvariable [namespace current]::var_${idx}(addr) \
      -relief flat -width 10 -justify right -state disabled -font $font
  }
  set var_${idx}(osize) $size
  if {$size=="menu"} {
    set var_${idx}(size) 32
    menubutton $w.size_$idx -textvariable [namespace current]::var_${idx}(size) \
        -indicatoron 1 -menu $w.size_$idx.menu -width 6\
        -relief raised -bd 2 -highlightthickness 0 -anchor c -pady -1p
    menu $w.size_$idx.menu -tearoff 0
    foreach i {8 16 32} {
      $w.size_$idx.menu add command -label $i \
          -command [namespace code "set var_${idx}(size) $i"]
    }
  } else {
    set var_${idx}(size) $size
    entry $w.size_$idx -textvariable [namespace current]::var_${idx}(size) \
        -relief flat -state disabled -width 8 -justify center -font $font
  }
  entry $w.hval_$idx -textvariable [namespace current]::var_${idx}(hval) \
      -relief sunken -width 12 -justify right -font $font
  bind $w.hval_$idx <Leave> [namespace code "h2d var_$idx"]
  bind $w.hval_$idx <Key-Return> [namespace code "h2d var_$idx"]
  entry $w.dval_$idx -textvariable [namespace current]::var_${idx}(dval) \
      -relief sunken -width 12 -justify right -font $font
  bind $w.dval_$idx <Leave> [namespace code "d2h var_$idx"]
  bind $w.dval_$idx <Key-Return> [namespace code "d2h var_$idx"]
  button $w.read_$idx -command [namespace code "read_var $idx"] -pady -2m \
      -text <<<<
  button $w.write_$idx -command [namespace code "write_var $idx"] -pady -2m \
      -text >>>>
  set var_${idx}(hval) ""
  set var_${idx}(dval) ""
  incr idx
}

proc rearrange {} {
  variable w
  variable idxs

  foreach slave [grid slaves $w] {
    grid forget $slave
  }
  grid  x x $w.lvalue -
  grid $w.lname $w.laddr $w.lsize $w.lvhex $w.lvdec $w.lread $w.lwrite

  foreach idx [lsort -integer $idxs] {
    grid $w.name_$idx $w.addr_$idx $w.size_$idx $w.hval_$idx $w.dval_$idx \
        $w.read_$idx $w.write_$idx -padx 5
  }
}

proc create {mainframe} {
  variable w
  variable idxs
  global global_setup_vmevars

  set idxs {}
  set w [frame $mainframe -relief flat -borderwidth 2]
  create_head
  foreach i [lsort -integer [array names global_setup_vmevars]] {
    create_varline [lindex $global_setup_vmevars($i) 0] \
        [lindex $global_setup_vmevars($i) 1] \
        [lindex $global_setup_vmevars($i) 2]
  }
  rearrange
  pack $w -side top -fill both
}

proc mk_setup {} {
  variable idxs
  global global_setup_extra
  global global_setup_vmevars

  ladd global_setup_extra global_setup_vmevars
  if [info exists global_setup_vmevars] {
    unset global_setup_vmevars
  }
  set n 0
  foreach i [lsort -integer $idxs] {
    variable var_$i
    set list {}
    lappend list [set var_${i}(name)]
    lappend list [set var_${i}(addr)]
    lappend list [set var_${i}(osize)]
    set global_setup_vmevars($n) $list
    incr n
  }
}

}
