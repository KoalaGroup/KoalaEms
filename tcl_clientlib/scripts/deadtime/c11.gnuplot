set title 'C11 dead time Distribution'
set data style lines
#set xlabel 'usec'
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

set term post enhanced color solid
set output 'c11_dt.ps'

plot [:2500][:.03]\
     'current_c11/e06_tdt' title 'total' ls 1\
    ,'current_c11/e02_ldt' title 'drams' ls 2\
    ,'current_c11/e03_ldt' title 'FB 3' ls 3\
    ,'current_c11/e04_ldt' title 'FB 4' ls 4\
    ,'current_c11/e06_ldt' title 'master' ls 5\
    ,'current_c11/e07_ldt' title 'scaler' ls 6

set term X
set output

replot
