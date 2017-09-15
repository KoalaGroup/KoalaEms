#
# global vars in this file:
#
# legend(win)
#

proc create_legend {} {
  global legend
  
  set legend(win) [text .legend -wrap word -height 2 -state disabled]
  return $legend(win)
}

proc legend_update {} {
  global hi legend

  $legend(win) configure -state normal
  $legend(win) delete 1.0 end
  set arrs [$hi(win) arrays]
  foreach i $arrs {
    if [$hi(win) arrcget $i -enabled] {
      set name [$i name]
      set color [$hi(win) arrcget $i -color]
      set exp [$hi(win) arrcget $i -exp]
      $legend(win) insert end "$name" $i
      if {$exp!=0} {
        $legend(win) insert end "(*10^$exp) " $i
      } else {
        $legend(win) insert end "  " $i
      }
      $legend(win) tag configure $i -foreground $color
    }
  }
  $legend(win) configure -state disabled
}
