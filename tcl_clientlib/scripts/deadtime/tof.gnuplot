set title 'TOF dead time Distribution, 2004-Oct-20 12:00'
#set title 'TOF dead time Distribution, 2004-Apr-12 23:43'
set data style lines
set xlabel 'deadtime/{/Symbol m}sec'
set mxtics 5
set mytics 10
set nologscale x
set nologscale y
set grid

set style line 1 lw 1
set style line 2 lw 1
set style line 3 lw 1
set style line 4 lw 1
set style line 5 lw 1
set style line 6 lw 1
set style line 7 lw 1
set style line 8 lw 1
set style line 9 lw 1
set style line 10 lw 1

set term post enh col solid
set output 'tof_dt.ps'

plot [0:250][0:] \
    'current_tof/t03_ldt' title 't03' ls 4,\
    'current_tof/t05_ldt' title 't05' ls 6,\
    'current_tof/t04_ldt' title 't04' ls 5,\
    'current_tof/t06_ldt' title 't06' ls 7,\
    'current_tof/t01_ldt' title 't01' ls 2,\
    'current_tof/t02_ldt' title 't02' ls 3,\
    'current_tof/t01_tdt' title 'total' ls 1

set term X
set output

replot
