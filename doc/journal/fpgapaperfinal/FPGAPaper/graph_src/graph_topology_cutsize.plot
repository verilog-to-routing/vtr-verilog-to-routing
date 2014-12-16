set terminal epslatex size 3.5in,2.5in color 3
set boxwidth 0.5
set xtics nomirror out rotate by 45 offset character 0,-1 right
set ytics nomirror axis out offset character -2,0
set style line 2 linetype 1 linecolor rgb '#9c9c9c'
set border 1+2 linestyle 2
set grid ytics linestyle 2
set format y '\footnotesize{%g}'
set nokey
set style fill solid 1.0

set output "graph_topology_cutsize.eps"
set xlabel '\footnotesize{\textbf{Graph Topology}}' offset character 0,-3
set ylabel '\footnotesize{\textbf{Hyperedge Cut Size}}' offset character -8,0
plot 'graph_topology_cutsize.dat' using 1:2:xtic(3) with boxes lt rgb '#4f81bd'
