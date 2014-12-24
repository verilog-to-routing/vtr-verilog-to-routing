set terminal epslatex size 3.5in,1.5in color 3
set boxwidth 0.5
set xtics nomirror out rotate by 45 offset character 0,-1 right
set ytics nomirror axis out offset character -2,0
set style line 2 linetype 1 linecolor rgb '#9c9c9c'
set border 1+2 linestyle 2
set grid ytics linestyle 2
set format y '\footnotesize{%g}'
set nokey
set style fill solid 1.0

set output "graph_topology_mcw.eps"
set xlabel '\footnotesize{\textbf{Graph Topology}}' offset character 0,-3
set ylabel '\footnotesize{\textbf{Minimum Channel Width}}' offset character -8,0
plot 'graph_topology_vpr.dat' using 1:3:xtic(2) with boxes lt rgb '#4f81bd'

set output "graph_topology_crit_path.eps"
set xlabel '\footnotesize{\textbf{Graph Topology}}' offset character 0,-3
set ylabel '\footnotesize{\textbf{Critical Path Delay}}' offset character -8,0
plot 'graph_topology_vpr.dat' using 1:4:xtic(2) with boxes lt rgb '#4f81bd'
