set terminal epslatex size 3.5in,2.0in color 3 solid
#set boxwidth 0.5
set xtics nomirror out rotate by 45 offset character 0,-1 right
set ytics nomirror out# offset character -14,0
set style line 2 linetype 1 linecolor rgb '#9c9c9c'
set border 1+2 linestyle 2
set grid ytics linestyle 2
set format y '\footnotesize{%g}'
set style fill pattern 2 border 0

set output "packing_bloat.eps"
set style data histogram
set xlabel '\footnotesize{\textbf{Circuit}}' offset character 0,-7
set ylabel '\footnotesize{\textbf{Blocks in Clustered Netlist}}' offset character -7,0
set key outside left top reverse horizontal Left spacing 2 width 25
plot 'packing_bloat.dat' using 2:xtic(1) title '\footnotesize{No Packing Constraints}' linecolor rgb '#4f81bd',\
'packing_bloat.dat' using 3:xtic(1) title '\footnotesize{Packing Constraints}' linecolor rgb '#C0504D'
