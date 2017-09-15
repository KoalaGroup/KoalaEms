set title 'ANKE dead time Distribution 2007-Mar-02'
set data style lines
#set xlabel 'usec'
set mxtics 10
set mytics 10
set logscale x
set nologscale y
set grid

# set linestyle 1 lw 2
# set linestyle 2 lw 2
# set linestyle 3 lw 2
# set linestyle 4 lw 2
# set linestyle 5 lw 2
# set linestyle 6 lw 2
# set linestyle 7 lw 2
# set linestyle 8 lw 2
# set linestyle 9 lw 2
# set linestyle 10 lw 2

set term post enhanced color solid
#set term post enhanced
set output 'anke_dt.ps'

# plot [0:140][:]\
#      'current_anke/p21b_tdt' title 'total' ls 1\
#     ,'current_anke/p21b_ldt' title 'VME p21' ls 2\
#     ,'current_anke/p16_ldt' title 'Scaler p16' ls 3\
#     ,'current_anke/p19_ldt' title 'FASTBUS p19' ls 4\
#     ,'current_anke/p26_ldt' title 'BW FERA p26' ls 5\
#     ,'current_anke/p23_ldt' title 'FERA p23' ls 6\
#     ,'current_anke/p27_ldt' title 'MWPC-FD p27' ls 7\
#     ,'current_anke/p34_ldt' title 'MWPC-3 p34' ls 8\
# 
plot [10:][:.01]\
     'current_anke/p21b_tdt' title 'total' \
    ,'current_anke/p21b_ldt' title 'VME p21' \
    ,'current_anke/p18_ldt' title 'Scaler p18' \
    ,'current_anke/p19_ldt' title 'FASTBUS p19' \
    ,'current_anke/p20_ldt' title 'p20' \
    ,'current_anke/p23_ldt' title 'BW FERA p23' \
    ,'current_anke/p27_ldt' title 'FWD p27' \
    ,'current_anke/p33_ldt' title 'NDMWPC p33' \
    ,'current_anke/p21b_ldt' title 'p21b' \
    ,'current_anke/p36_ldt' title 'ND p36' \
    ,'current_anke/p34_ldt' title 'FWDF1 p34' \
    ,'current_anke/p16_ldt' title 'Scaler p16' \

# plot [0:150][:]\
     'current_anke/p21b_tdt' title 'total' \
    ,'current_anke/p18_ldt' title 'Scaler p18' \
    ,'current_anke/p19_ldt' title 'FASTBUS p19' \
    ,'current_anke/p23_ldt' title 'BW FERA p23' \
    ,'current_anke/p27_ldt' title 'FWD p27' \
    ,'current_anke/p33_ldt' title 'NDMWPC p33' \
    ,'current_anke/p34_ldt' title 'FWDF1 p34' \

set term X
set output

replot
