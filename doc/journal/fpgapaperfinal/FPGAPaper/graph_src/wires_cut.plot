set terminal epslatex size 3.5in,1.5in color 3
set xtics nomirror out
set ytics nomirror out
set style line 3 linetype 1 linecolor rgb '#9c9c9c'
set border 1+2 linestyle 3
set grid ytics linestyle 3
set format x '\footnotesize{%g}'
set format y '\footnotesize{%g}'
set key outside left top reverse horizontal Left spacing 2 width 20

set style line 1 linetype 1 linecolor rgb '#4f81bd' linewidth 5 pointtype 5 pointsize 1
set style line 2 linetype 1 linecolor rgb '#c0504d' linewidth 5 pointtype 13 pointsize 1.5

set output "wires_cut.eps"
set xlabel '\footnotesize{\textbf{\% wires cut}}' offset character 0,-2
set ylabel '\footnotesize{\textbf{Minimum Channel Width}}' offset character -5,0
plot 'wires_cut.dat' using 1:2 with linespoints title '\footnotesize{Best CAD Flow}' linestyle 1, 'wires_cut.dat' using 1:4 with linespoints title '\footnotesize{Best CAD Flow in \cite{interposer2014}}' linestyle 2
