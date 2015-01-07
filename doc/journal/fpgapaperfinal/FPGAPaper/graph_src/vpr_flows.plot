set terminal epslatex size 3.5in,2.0in color 3 solid
set xtics nomirror out rotate by 45 offset character 0,-1 right
set ytics nomirror out# offset character -14,0
set style line 2 linetype 1 linecolor rgb '#9c9c9c'
set border 1+2 linestyle 2
set grid ytics linestyle 2
set format y '\footnotesize{%g}'
set style fill pattern 2 border 0

set tmargin 9.5
set output "vpr_flows_mcw.eps"
set style data histogram
set xlabel '\footnotesize{\textbf{Circuit}}' offset character 0,-7
set ylabel '\footnotesize{\textbf{Minimum Channel Width}}' offset character -5,0
set key outside left reverse horizontal Left spacing 2 width 20
plot 'vpr_flows_mcw.dat' using 3:xtic(2) title '\footnotesize{No Partitioning (Base Flow)}' linecolor rgb '#4f81bd',\
'vpr_flows_mcw.dat' using 5:xtic(2) title '\footnotesize{Packing Constraints Only}' linecolor rgb '#C0504D',\
'vpr_flows_mcw.dat' using 4:xtic(2) title '\footnotesize{Placement Constraints Only}' linecolor rgb '#9BBB59',\
'vpr_flows_mcw.dat' using 6:xtic(2) title "\\footnotesize{Packing and Placement}\n\n\\footnotesize{Constraints}" linecolor rgb '#8064A2'

set output "vpr_flows_crit_path.eps"
set ylabel '\footnotesize{\textbf{Critical Path Delay (ns)}}' offset character -5,0
plot 'vpr_flows_crit_path.dat' using 3:xtic(2) title '\footnotesize{No Partitioning (Base Flow)}' linecolor rgb '#4f81bd',\
'vpr_flows_crit_path.dat' using 5:xtic(2) title '\footnotesize{Packing Constraints Only}' linecolor rgb '#C0504D',\
'vpr_flows_crit_path.dat' using 4:xtic(2) title '\footnotesize{Placement Constraints Only}' linecolor rgb '#9BBB59',\
'vpr_flows_crit_path.dat' using 6:xtic(2) title "\\footnotesize{Packing and Placement}\n\n\\footnotesize{Constraints}" linecolor rgb '#8064A2'
