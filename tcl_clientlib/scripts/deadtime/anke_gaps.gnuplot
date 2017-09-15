set title 'ANKE trigger Distribution, 2007-Feb-28'
set data style lines
#set linestyle 1 linewidth 11
set xlabel 'trigger distance/{/Symbol m}sec'
set mxtics 10
set mytics 10
set nologscale x
set logscale y
set nogrid
set grid xtics mxtics

#set linestyle 1 lw 1

set term post enh col solid
set output 'anke_gaps.ps'

plot [0:6553.6][:] \
    'current_anke/p21b_gaps'

set term X
set output

replot
