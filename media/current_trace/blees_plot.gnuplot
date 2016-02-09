set terminal postscript enhanced eps color font "Helvetica,16" size 6in,4in

set datafile separator ","
set output "blees_current_trace.eps"

set style line 1 lt 1  ps 1.5 pt 7 lw 5 lc rgb "#d7191c"
set style line 2 lt 1  ps 1.0 pt 7 lw 5 lc rgb "#fdae61"
set style line 3 lt 1  ps 1.2 pt 7 lw 5 lc rgb "#abdda4"
set style line 4 lt 1  ps 1.0 pt 7 lw 3 lc rgb "#2b83ba"
set style line 5 lt 3  ps 1.2 pt 7 lw 3 lc rgb "#000000"
set style line 6 lt 3  ps 1.0 pt 7 lw 3 lc rgb "#888888"

set border 3

set xlabel "Time (s)"
set xtics nomirror
set xrange [0:5]

set ylabel "Current Draw (mA)"
set ytics nomirror
set yrange [0:10]

unset key

# label advertisements
set label at 0.5,9.5 "Advertisements"
set arrow from 0.8,9.3 to 0.9,8.0 ls 6
set arrow from 0.8,9.3 to 1.9,8.0 ls 6
set arrow from 0.8,9.3 to 2.9,8.0 ls 6
set arrow from 0.8,9.3 to 3.9,8.0 ls 6

# label sensor data collection
set label at 1.03,4 "Sensor Sampling"
set arrow from 1.33,3.8 to 1.85,1.0 ls 6
set arrow from 1.33,3.8 to 4.85,1.0 ls 6

plot 'blees_current_trace.csv' u ($1):($2) w l ls 4

