set title 'C11 trigger Distribution'
set data style lines
#set xlabel 'usec'
set mxtics 10
set mytics 10
set nologscale x
set nologscale y
set grid

set linestyle 1 lw 1

set term post enhanced color solid
set output 'c11_gaps.ps'

plot [:7000][:.00016]\
     'current_c11/e06_gaps' ls 1

set term X
set output

replot
