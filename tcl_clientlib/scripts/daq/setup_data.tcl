# $ZEL: setup_data.tcl,v 1.22 2010/05/05 19:40:11 wuestner Exp $
# copyright: 1998, 2000, 2001
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_setup(super_setup); zu lesender supersetupfile
# global_setupdir; directory of supersetup
# global_setupdata(names); Namen der VED-Namespaces
#

proc clear_setup_data {} {
  global global_setupdata

  if [info exists global_setupdata(names)] {
    foreach i $global_setupdata(names) {
      if [catch {interp delete ved_setup_$i} mist] {
        output_append "$i does not exist" tag_red
      }
    }
  }
  if [catch {interp delete mastersetup} mist] {
    output_append "mastersetup does not exist" tag_red
  }
  if [info exists global_setupdata] {unset global_setupdata}
  change_status uninitialized "Setup cleared"
}

proc check_need_reed {space file} {
  return 1
# wenn mastersetup geaendert wurde, muss alles neu gelesen werden!!
  if {![interp exists $space]} {return 1}
  set read_always [$space eval {[info exists read_always] && $read_always}]
  if {$read_always} {return 1}
  if {[$space eval filetimes_changed sourced_files]} {return 1}
  return 0
}

proc pre_read_funcs {space} {
  if [mastersetup eval info exists export_procs] {
    foreach i [mastersetup eval set export_procs] {
      interp alias $space mastersetup_$i mastersetup $i
    }
  }
  if [mastersetup eval info exists export_vars] {
    foreach i [mastersetup eval set export_vars] {
      $space eval set $i \{[mastersetup eval set $i]\}
    }
  }
}

proc read_setup_file {space filename ident} {
  global global_setupdir global_verbose

  $space eval set setupdir $global_setupdir
  $space eval set filename $filename
  $space eval set spacename $space
  $space eval set global_verbose $global_verbose
  $space eval set ident $ident
  if [info exists env(TMPDIR)] {
    $space eval set tmpdir $env(TMPDIR)
  } else {
    $space eval set tmpdir /tmp
  }
  $space alias output sandbox_output $ident
  $space alias output_append sandbox_output_append $ident
  $space alias output_append_nonl output_append_nonl
  $space alias log_see_end log_see_end
  $space alias strerror_bsd strerror_bsd
  $space alias source sandbox_source $space
  $space alias open sandbox_open $space
  #$space alias creat sandbox_creat $space
  $space expose glob
  $space alias get_run_nr ::run_nr::get_run_nr

  set res [$space eval {
    if [catch {source $filename} mist] {
      output "read setupfile ($filename): $mist" tag_red
      output "$errorInfo" tag_blue
      output "ERROR; return -1 (read_setup_file)" tag_red
      return -1
    }
#     proc ident {} {
#       global spacename
#       return $spacename
#     }
    proc bgerror {args} {
      global errorCode errorInfo
      output "background error in slave:" tag_blue
      if {$errorCode!="NONE"} {output_append "errorCode: $errorCode" tag_blue}
      output_append $errorInfo tag_blue
    }

#     foreach i [info vars] {
#       if {[array exists $i]} {
#         output_append "array $i"
#         set size [array size $i]
#         if {$size>5} {
#           output_append "  size=$size"
#         } else {
#           foreach j [lsort [array names $i]] {
#             output_append "  $j: [eval set $i\($j\)]"
#           }
#         }
#       } else {
#         output_append "$i: [set $i]"
#       }
#     }
    return 0
  }]
  if {$res<0} {return -1}
  return 0
}

proc extend_permissions {space} {
  if {[$space eval {info exists extended_permissions}]} {
    if {[$space eval set extended_permissions]==0xDEADBEEF} {
      output_append "  extending permissions for $space" tag_yellow
      $space alias creat sandbox_creat $space
      $space expose file
      $space expose exec
      $space alias get_vedcommand sandbox_get_vedcommand $space
    }
  }
}

proc read_setup_data {} {
  global global_setup global_setupdata
  global global_setupdir

# super_setup-file geaendert oder nie gelesen?
  set need_read [check_need_reed mastersetup $global_setup(super_setup)]

# read super_setup file if necessary
  if {$need_read} {
    output "read master setup file ($global_setup(super_setup))"
    set global_setupdir [file dirname $global_setup(super_setup)]

    catch {interp delete mastersetup}
    interp create -safe mastersetup
    if {[read_setup_file mastersetup $global_setup(super_setup) master]<0} {
      output "ERROR; return -1 (read_setup_data 1)" tag_red
      return -1
    }

    if {![mastersetup eval array exists setupfile]} {
      output "master setup file ($global_setup(super_setup))\
        does not set array setupfile" tag_red
      output "ERROR; return -1 (read_setup_data 2)" tag_red
      return -1
    }
  } else {
    output "no need to read master setup file\
      ($global_setup(super_setup))"
  }

  set ved_idx 0
  set cc_idx 0
  set global_setupdata(names) {}
  foreach i [lsort -dictionary [mastersetup eval array names setupfile]] {
    lappend global_setupdata(names) $i
  }
  log_see_end

# read individual setup files if necessary  
  output_append "read setupfiles:"
  foreach i $global_setupdata(names) {
    set filename [file join $global_setupdir \
        [mastersetup eval set setupfile($i)]]
    set need_read [check_need_reed ved_setup_$i $filename]

    if {$need_read} {
      output_append "  $filename"
      catch {interp delete ved_setup_$i}
      interp create -safe ved_setup_$i
      ved_setup_$i eval set ved_idx $ved_idx
      ved_setup_$i eval set cc_idx $cc_idx; # wrong for em
      pre_read_funcs ved_setup_$i
      if {[read_setup_file ved_setup_$i $filename $i]<0} {return -1}

      ved_setup_$i eval  {
        if {![info exists description]} {set description "no Description"}
      }

      set is_em 0
      if {[ved_setup_$i eval info exists eventbuilder]} {
        set is_em [ved_setup_$i eval set eventbuilder]
      }
      if {!$is_em} {
        incr cc_idx
      }

      extend_permissions ved_setup_$i
    } else {
      output_append "  no need to read $filename"
    }
    log_see_end
    incr ved_idx
  }
  output_append "read setupfiles:"
  return 0
}
