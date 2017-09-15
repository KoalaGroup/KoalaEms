proc autorestore {} {
  global global_setup hi global_autorestore global_arrays

  if {[info exists global_autorestore]} {
    foreach aname [array names global_autorestore] {
      if [catch {set file [open $global_autorestore($aname)_info "RDONLY"]} mist] {
        puts $mist
      } else {
        set arr [gets $file]
        set name [gets $file]
        set limit [gets $file]
        set color [gets $file]
        histoarray $arr $name
        set global_arrays($arr) ""
        $arr limit $limit
        close $file
        $arr restore $global_autorestore($aname)
        $hi(win) attach $arr -color $color
        set max [$arr gettime [expr [$arr size]-1]]
        set imax [lindex [split $max .] 0]
        $hi(win) xmax $imax
      }
    }
    legend_update
  }
}

proc autostart {} {
  global global_setup global_autostart global_exec

  if {[info exists global_autostart]} {
    foreach i [array names global_autostart] {
      if {[string index $global_autostart($i) 0]!="#"} {
        regsub {%port} $global_autostart($i) $global_setup(port) line
        regsub {%host} $line [exec hostname] line
        if [catch {set pid [eval "exec $line &"]} mist] {
          bgerror $mist
        } else {
          foreach id $pid {lappend global_exec $id}
        }
      }
    }
  }
}

proc xh_start {} {
  global global_setup

  if {$global_setup(data_autorestore)} {autorestore}
  if {$global_setup(autostart)} {autostart}
}

proc data_autosave {} {
  global global_setup hi global_autorestore
  
  if {[info exists global_setup(extra)]} {
    set idx [lsearch -exact $global_setup(extra) global_autorestore]
    if {$idx!=-1} {
      set global_setup(extra) [lreplace $global_setup(extra) $idx $idx]
    }
  }
  if {[info exists global_autorestore]} {unset global_autorestore}

  foreach arr [$hi(win) arrays] {
    regsub -all { } "$global_setup(data_autodir)/[winfo name .]_$arr" {} fname
    set file [open ${fname}_info "CREAT WRONLY"]
    puts $file $arr
    puts $file [$arr name]
    puts $file [$arr limit]
    puts $file [$hi(win) arrcget $arr -color]
    close $file
    $arr dump $fname
    set global_autorestore($arr) $fname
  }
  if {[info exists global_autorestore]} {
    lappend global_setup(extra) global_autorestore
  }
}

proc xh_end {} {
  global global_setup global_setup_file global_exec
  if [info exists global_exec] {
    foreach pid $global_exec {
      catch {exec kill $pid}
    }
  }
  if {$global_setup(data_autosave)} {data_autosave}
  if {$global_setup(autosave)} {save_setup_default}
  exit
}
