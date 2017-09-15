set title 'GEM trigger Distribution'
set data style lines
set linestyle 1 linewidth 11
set xlabel 'trigger distance/{/Symbol m}sec'
set mxtics 10
set mytics 10
set nologscale x
set nologscale y
set grid

set linestyle 1 lw 1

set term post enh col solid
set output 'gem_gaps.ps'

plot [:10][:] \
    'current_gem/g02_gaps' ls 1

set term X
set output

replot
