set terminal postscript eps enhanced "Helvetica" 24
set output 'sim2.eps'
set key bottom right Right
set xlabel "Number of speakers in database"
set ylabel 'Online computation/communication (s)'
set log y
set xtics 0,200,1000
set xtics add ("10" 10)
set ytics add ("0.05" 0.05)
set ytics add ("" 1)
set ytics add ("1.3" 1.3)
set ytics add ("2" 2)
set ytics add ("50" 50)
set ytics add ("" 100)
set ytics add ("130" 130)

plot [:] [0.04:130] 'onl_comp.csv' using 1:(($2+$5)/1000+$6/1024) title 'Ours' with linespoints pt 5 ps 2, 'out.csv' using 1:($2/1000+$3/128) title 'Treiber et al.' with linespoints pt 6 ps 2
