# $ZEL: select_new_dataout.tcl,v 1.3 2003/02/04 19:27:57 wuestner Exp $
# copyright: 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc replace_dataout {space} {
  set res [$space eval {
    foreach alist [array names dataout_autochange] {
      set idx [lsearch -exact $dataout_autochange($alist) $dataout_with_full_tape]
      if {$idx>=0} {
        if {$dataout_with_full_tape!=$active_tape($alist)} {
          output "tape to be changed ($dataout_with_full_tape) != last active tape ($active_tape($i))"
        }
        set autochange_list [lreplace $dataout_autochange($alist) $idx $idx]
        break
      }
    }
    output "new list of dataouts for autochange: $autochange_list"
    # search for new tape
    set active_tape($alist) none
    foreach idx $autochange_list {
      global global_dataout_${ved}_${idx}
      if [set global_dataout_${ved}_${idx}(istape)] {
        if [catch {set s [$ved command1 TapeReadPos $idx]} mist] {
          output "read position $ved $idx: $mist"
          set bop 0
        } else {
          set bop [lindex $s 2]
        }
        if {$bop} {
          set active_tape($alist) $idx
          break
        }
      } else {
        output "dataout ${idx} of VED ${ved} is not a tape!" tag_red
      }
    }
    if {$active_tape($alist)=="none"} {
      output "No tape found at BOP; giving up" tag_red
      return -1
    } else {
      output "selected DO $active_tape($alist) for VED $ved list $autochange_list"
      $ved dataout enable $active_tape($alist) 1
    }
  return 0
  }]
  return $res
}

proc select_dataouts {space} {
  set res [ved_setup_$space eval {
    foreach alist [array names dataout_autochange] {
      set autochange_list $dataout_autochange($alist)
      foreach idx $autochange_list {
        $ved dataout enable $idx 0
      }
      set active_tape($alist) none
      foreach idx $autochange_list {
        global global_dataout_${ved}_${idx}
        if [set global_dataout_${ved}_${idx}(istape)] {
          if [catch {set s [$ved command1 TapeReadPos $idx]} mist] {
            output "read position $ved $idx: $mist"
            set bop 0
          } else {
            set bop [lindex $s 2]
          }
          if {$bop} {
            set active_tape($alist) $idx
            break
          }
        } else {
          output "dataout ${idx} of VED ${ved} is not a tape!" tag_red
        }
      }
      if {$active_tape($alist)=="none"} {
        output "No tape found at BOP; giving up" tag_red
        return -1
      } else {
        output "selected do $active_tape($alist) for VED $ved list $autochange_list"
        $ved dataout enable $active_tape($alist) 1
      }
    }
  return 0
  }]
  return $res
}

proc select_initial_dataouts {} {
  global global_setupdata

  foreach space $global_setupdata(names) {
    set res [select_dataouts $space]
    if {$res<0} {return -1}
  }
  return 0
}

proc select_new_dataout {ved do_idx} {
  global global_interps

  set space $global_interps($ved)
  $space eval set dataout_with_full_tape $do_idx
  set res [replace_dataout $space]
  return $res
}
