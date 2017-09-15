set title 'GEM dead time Distribution'
set data style lines
set linestyle 1 linewidth 11
set xlabel 'deadtime/{/Symbol m}sec'
set mxtics 10
set mytics 10
set nologscale x
set nologscale y
set grid

set linestyle 1 lw 1
set linestyle 2 lw 1
set linestyle 3 lw 1
set linestyle 4 lw 1
set linestyle 5 lw 1
set linestyle 6 lw 1
set linestyle 7 lw 1
set linestyle 8 lw 1
set linestyle 9 lw 1
set linestyle 10 lw 1

set term post enh col solid
set output 'gem_dt.ps'

plot [0:250][:] \
    'current_gem/g02_ldt' title 'g02' ls 4,\
    'current_gem/g03_ldt' title 'g03' ls 6

set term X
set output

replot
