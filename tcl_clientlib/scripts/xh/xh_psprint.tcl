proc dataprint_psprint {} {
  global static_dataprint
  global static_dataprint_enable static_dataprint_color static_dataprint_dash
  global hi

  if [
    catch {
      if {$static_dataprint(pf)=="print"} {
        set file [open "|$static_dataprint(lpr)" "WRONLY"]
      } else {
        set file [open $static_dataprint(fname) "CREAT WRONLY TRUNC"]
      }
    } mist
  ] {
    bgerror $mist
    return
  }
  
# header
  puts $file {%!}
  puts $file {%}
  puts $file {%% Description: Output from XHisto}
  puts $file {%% Copyright 1996 Peter Wuestner, Forschungszentrum Juelich, ZEL}
  puts $file {%% xh 1.2}
  puts $file {%% EndComments}
  puts $file {}

  puts $file {% strwidth}
  puts $file {% string --> width}
  puts $file "/strwidth \{"
  puts $file {  gsave}
  puts $file {  0 0 moveto}
  puts $file {  false charpath flattenpath pathbbox}
  puts $file {  pop exch pop sub neg}
  puts $file {  grestore}
  puts $file "\} def"
  puts $file {}

  puts $file {% strheight}
  puts $file {% string --> height}
  puts $file "/strheight \{"
  puts $file {  gsave}
  puts $file {  0 0 moveto}
  puts $file {  false charpath flattenpath pathbbox}
  puts $file {  exch pop sub neg exch pop}
  puts $file {  grestore}
  puts $file "\} def"
  puts $file {}

  puts $file {% show left centered}
  puts $file "/show_lc \{"
  puts $file {  gsave}
  puts $file {  2 index}
  puts $file {  0 0 moveto}
  puts $file {  false charpath flattenpath pathbbox}
  puts $file {  3 2 roll}
  puts $file {  add 2 div}
  puts $file {  3 1 roll}
  puts $file {  pop}
  puts $file {  3 1 roll}
  puts $file {  sub}
  puts $file {  3 1 roll}
  puts $file {  sub}
  puts $file {  exch}
  puts $file {  grestore}
  puts $file {  moveto show}
  puts $file "\} def"
  puts $file {}

  puts $file {% show right centered}
  puts $file "/show_rc \{"
  puts $file {  gsave}
  puts $file {  2 index}
  puts $file {  0 0 moveto}
  puts $file {  false charpath flattenpath pathbbox}
  puts $file {  3 2 roll}
  puts $file {  add 2 div}
  puts $file {  3 1 roll}
  puts $file {  exch}
  puts $file {  pop}
  puts $file {  3 1 roll}
  puts $file {  sub}
  puts $file {  3 1 roll}
  puts $file {  sub}
  puts $file {  exch}
  puts $file {  grestore}
  puts $file {  moveto show}
  puts $file "\} def"
  puts $file {}

  puts $file {% show centered top}
  puts $file "/show_ct \{"
  puts $file {  gsave}
  puts $file {  2 index}
  puts $file {  0 0 moveto}
  puts $file {  false charpath flattenpath pathbbox}
  puts $file {  3 2 roll}
  puts $file {  pop}
  puts $file {  3 1 roll}
  puts $file {  add 2 div}
  puts $file {  3 1 roll}
  puts $file {  sub}
  puts $file {  3 1 roll}
  puts $file {  sub}
  puts $file {  exch}
  puts $file {  grestore}
  puts $file {  moveto show}
  puts $file "\} def"
  puts $file {}

  puts $file {% show centered centered}
  puts $file {% z.B. (mist) 0 0 show_cc}
  puts $file "/show_cc \{"
  puts $file {  gsave}
  puts $file {  2 index}
  puts $file {  0 0 moveto}
  puts $file {  false charpath flattenpath pathbbox}
  puts $file {  3 2 roll}
  puts $file {  add 2 div}
  puts $file {  3 1 roll}
  puts $file {  add 2 div}
  puts $file {  3 1 roll}
  puts $file {  sub}
  puts $file {  3 1 roll}
  puts $file {  sub}
  puts $file {  exch}
  puts $file {  grestore}
  puts $file {  moveto show}
  puts $file "\} def"
  puts $file {}

#  puts $file "/xtrans \{"
#  puts $file {  xmin sub}
#  puts $file {  xdiff div w mul}
#  puts $file "\} def"
#  puts $file {}

  puts $file "/xtrans \{"
  puts $file {  xmax div w mul}
  puts $file "\} def"
  puts $file {}

  puts $file "/ytrans \{"
  puts $file {  ymax div h mul}
  puts $file "\} def"
  puts $file {}

  puts $file "/xtic \{"
  puts $file {% x length xtic}
  puts $file {  exch}
  puts $file {  xtrans}
  puts $file {  2 copy}
  puts $file {  0 moveto}
  puts $file {  0 exch neg rlineto}
  puts $file {  stroke}
  puts $file {  h moveto}
  puts $file {  0 exch rlineto}
  puts $file {  stroke}
  puts $file "\} def"
  puts $file {}

  puts $file "/xlab \{"
  puts $file {% label x xlab}
  puts $file {  xtrans}
  puts $file {  ltic 3 add neg}
  puts $file {  show_ct}
  puts $file "\} def"
  puts $file {}

  puts $file "/ytic \{"
  puts $file {% y length ytic}
  puts $file {  exch}
  puts $file {  ytrans}
  puts $file {  2 copy}
  puts $file {  0 exch moveto}
  puts $file {  neg 0 rlineto}
  puts $file {  stroke}
  puts $file {  w exch moveto}
  puts $file {  0 rlineto}
  puts $file {  stroke}
  puts $file "\} def"
  puts $file {}

#   puts $file "/ylab \{"
#   puts $file {% label y ylab}
#   puts $file {  ytrans}
#   puts $file {  w ltic 3 add add}
#   puts $file {  exch}
#   puts $file {  show_lc}
#   puts $file "\} def"
#   puts $file {}
  puts $file "/ylab \{"
  puts $file {% label y ylab}
  puts $file {  ytrans}
  puts $file {  1 index 1 index}
  puts $file {  w ltic 3 add add}
  puts $file {  exch}
  puts $file {  show_lc}
  puts $file {  0 ltic 3 add sub}
  puts $file {  exch}
  puts $file {  show_rc}
  puts $file "\} def"
  puts $file {}

  puts $file "/mto \{"
  puts $file {% x y mto}
  puts $file {  exch xtrans}
  puts $file {  exch ytrans}
  puts $file {  1 index exch}
  puts $file {  moveto}
  puts $file "\} def"
  puts $file {}
  puts $file "/lto \{"
  puts $file {% x y lto}
  puts $file {  exch xtrans}
  puts $file {  exch ytrans}
  puts $file {  3 -1 roll 1 index}
  puts $file {  lineto}
  puts $file {  1 index exch}
  puts $file {  lineto}
  puts $file "\} def"
  puts $file {}

  puts $file "/setlegendvals \{"
  puts $file {/leglines 3 def}
  puts $file {/legsp1 4 def}
  puts $file {/legsp2 8 def}
  puts $file {/legllen 16 def}
  puts $file {/leglheight (S) strheight 2 mul def}
  puts $file {  0}
  foreach arr $static_dataprint(arrs) {
    if {$static_dataprint_enable($arr)} {
      puts $file "  ([$arr name]) strwidth"
      puts $file {  legsp1 add legsp2 add legllen add}
      puts $file {  exch 1 index add}
      puts $file {  dup w lt}
      puts $file "    \{exch pop\}"
      puts $file "    \{exch pop /leglines leglines 1 add def\}"
      puts $file {  ifelse}
      puts $file {}
    }
  }
  puts $file {  /legheight leglheight leglines mul def}
  puts $file "\} def"
  puts $file {}

  puts $file "/printlegend \{"
  puts $file {/legx 0 def}
  puts $file {/legy 0 yrand sub leglheight sub def}
  foreach arr $static_dataprint(arrs) {
    if {$static_dataprint_enable($arr)} {
      if {$static_dataprint(color)} {
        set RGB [color2rgb $static_dataprint_color($arr)]
        puts $file "$RGB setrgbcolor"
      }
      if {$static_dataprint(dash)} {
        puts $file "\[$static_dataprint_dash($arr)\] 0 setdash"
      }
      set expo [$hi(win) arrcget $arr -exp]
      if {$expo!=0} {
        set text "[$arr name](*10^$expo)"
      } else {
        set text [$arr name]
      }
      puts $file {legx legllen add legsp1 add}
      puts $file "($text) strwidth add legsp2 add"
      puts $file "w ge \{/legx 0 def /legy legy leglheight sub def\} if"
      puts $file {legx legy moveto}
      puts $file {legllen 0 rlineto stroke}
      puts $file {/legx legx legllen add legsp1 add def}
      puts $file "($text) legx legy show_lc"
      puts $file "/legx legx ($text) strwidth add legsp2 add def"
      puts $file {}
    }
  }
  puts $file "\} def"
  puts $file {}

# definitionen
  puts $file {/Courier findfont}
  puts $file {8 scalefont setfont}
  puts $file {}
  puts $file "/pagewidth $static_dataprint(width) def"
  puts $file "/pageheight $static_dataprint(height) def"
  puts $file {}
  puts $file {/xoffs 36 def}
  puts $file {/yoffs 36 def}
  puts $file {}
  puts $file {/width pagewidth xoffs 2 mul sub def}
  puts $file {/height pageheight yoffs 2 mul sub def}
  puts $file {}
  puts $file {/xrand 50 def}
  puts $file {/yrand 20 def}
  puts $file {}
#  puts $file "/xmin $static_dataprint(t0_i) def"
  set x0 $static_dataprint(t0_i)
#  puts $file "/xmax $static_dataprint(t1_i) def"
  puts $file "/xmax [expr $static_dataprint(t1_i)-$x0] def"
#  puts $file {/xdiff xmax xmin sub def}
  puts $file "/ymax $static_dataprint(y1) def"
  puts $file {}
  puts $file {/ltic 8 def}
  puts $file {/stic 5 def}
  puts $file {}

# auesserer Rahmen
  switch -exact $static_dataprint(rotation) {
    0 {}
    90 {
      puts $file {90 rotate}
      puts $file {0 pageheight neg translate}
      puts $file {}
    }
    180 {}
    270 {}
  }

  puts $file {xoffs yoffs translate}
  puts $file {0 setgray}
  puts $file {0.1 setlinewidth}
  puts $file {0 0 moveto}
  puts $file {0 height lineto}
  puts $file {width height lineto}
  puts $file {width 0 lineto}
  puts $file {closepath stroke}
  puts $file {}

# innerer Rahmen
  puts $file {/w width xrand 2 mul sub def}
# wenn man eine Legende haben will, kommt unten noch etwas hinzu:
  puts $file {setlegendvals}
  puts $file {/h height yrand 2 mul legheight add sub def}
  puts $file {xrand yrand legheight add translate}
  puts $file {0 0 moveto}
  puts $file {0 h lineto}
  puts $file {w h lineto}
  puts $file {w 0 lineto}
  puts $file {closepath stroke}
  puts $file {}
  puts $file {gsave}
  puts $file {printlegend}
  puts $file {grestore}

# xtics
  switch -exact $static_dataprint(xticunit) {
    seconds {set ut 1}
    minutes {set ut 60}
    hours {set ut 3600}
    days {set ut 86400}
  }
  switch -exact $static_dataprint(xlabunit) {
    seconds {set ul 1}
    minutes {set ul 60}
    hours {set ul 3600}
    days {set ul 86400}
  }
#  set x0 $static_dataprint(t0_i)  
  set xt [expr $static_dataprint(xtics)*$ut]
  set xl [expr $static_dataprint(xlabs)*$ul]
  for {set x [expr $x0-$x0%$xt+$xt]} {$x<=$static_dataprint(t1_i)} {incr x $xt} {
    puts $file "[expr $x-$x0] stic xtic"
  }
  puts $file {}

# xlabels
  for {set x [expr $x0-$x0%$xl+$xl]} {$x<=$static_dataprint(t1_i)} {incr x $xl} {
    puts $file "[expr $x-$x0] ltic xtic"
    puts $file "([clock format $x -format $static_dataprint(xform)]) [expr $x-$x0] xlab"
  }
  puts $file {}

# ytics
  for {set y 0} {$y<=$static_dataprint(y1)} {incr y $static_dataprint(ytics)} {
    puts $file "$y stic ytic"
  }
  puts $file {}

# ylabels
  for {set y 0} {$y<=$static_dataprint(y1)} {incr y $static_dataprint(ylabs)} {
    puts $file "$y ltic ytic"
    puts $file "($y) $y ylab"
  }
  puts $file {}

  puts $file {gsave}
# clippath
  puts $file {0 0 moveto}
  puts $file {0 h lineto}
  puts $file {w h lineto}
  puts $file {w 0 lineto}
  puts $file {closepath clip}
  puts $file {newpath}
  puts $file {}

# daten
  puts $file {gsave}
#  puts $file "w xmax div h ymax div scale"
  puts $file {0.1 setlinewidth}

  foreach arr $static_dataprint(arrs) {
    if {$static_dataprint_enable($arr)} {
      set expo [$hi(win) arrcget $arr -exp]
      set factor [expr pow(10,$expo)]
      if {$static_dataprint(color)} {
        set RGB [color2rgb $static_dataprint_color($arr)]
        puts $file "$RGB setrgbcolor"
      }
      if {$static_dataprint(dash)} {
        puts $file "\[$static_dataprint_dash($arr)\] 0 setdash"
      }
      #set i0 [$arr rightindex $static_dataprint(t0_i)]
      #set i1 [$arr leftidx $static_dataprint(t1_i)]
      set i0 [$arr leftidx $static_dataprint(t0_i)]
      set i1 [$arr rightindex $static_dataprint(t1_i)]
      set i $i0
      #puts $file "[$arr gettime $i %d] xtrans [$arr getval $i] ytrans moveto"
      #for {incr i} {$i<=$i1} {incr i} {
      #  puts $file "[$arr gettime $i %d] xtrans [$arr getval $i] ytrans lineto"
      #}
      puts $file "[expr [$arr gettime $i]-$x0] [expr [$arr getval $i]*$factor] mto"
      for {incr i} {$i<=$i1} {incr i} {
        puts $file "[expr [$arr gettime $i]-$x0] [expr [$arr getval $i]*$factor] lto"
      }
      puts $file {stroke}
      puts $file {}
    }
  }
  puts $file {grestore}

  puts $file {grestore}
# ende
  if {$static_dataprint(eps)=="ps"} {
    puts $file "showpage"
  }

  close $file
}
