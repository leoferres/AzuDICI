# Gnuplot Script
# To generate: C-x p
################################################################################
# This program by L A Ferres is in the public domain and freely copyable.
# (or, if there were problems, in the errata  --- see
#    http://www.udec.cl/~leo/programs.html)
# If you find any bugs, please report them immediately to: leo@inf.udec.cl
################################################################################
#
# FILE NAME: plingeling_6cores.gnuplot
#
# PURPOSE:
#
# FILE(S) GENERATING THIS DATA:
#
# CREATION DATE: 2012-02-02-13:36; MODIFIED:
#
################################################################################
#
# General options for Azu graphs
# They are inserted in Latex files in a picture environment
#
set terminal postscript portrait "Times-Roman" 10
set terminal postscript enhanced
set output "plingeling_6cores.eps"
set border
set size square 0.5,0.5 # percent of original (50% of its size here)
unset time # unset for no time

# Details for this particular plot
set key left # legend to the left (bottom is also an option)
unset logscale y  #set for logarithmic scale y (add x for log x)
set xlabel "# of threads" # XAxis title
set ylabel "Speedup" # YAxis title
set key at -.7,29
set border 3 lw 0.5
set xtics nomirror
set ytics nomirror
plot "plingeling_6cores.dat" using 1:2 lt 2 lw 0.5 ps 0.5 title '{/Times=6 goldb-heqc-i8mul}' with linespoints, \
"plingeling_6cores.dat" using 1:3 lt 2 lw 0.5 ps 0.5 title '{/Times=6 hoons-vbmc-s04-06}' with linespoints, \
"plingeling_6cores.dat" using 1:4 lt 2 lw 0.5 ps 0.5 title '{/Times=6 manol-pipe-c6bid_i}' with linespoints, \
"plingeling_6cores.dat" using 1:5 lt 2 lw 0.5 ps 0.5 title '{/Times=6 manol-pipe-c7nidw}' with linespoints, \
"plingeling_6cores.dat" using 1:6 lt 2 lw 0.5 ps 0.5 title '{/Times=6 manol-pipe-g10idw}' with linespoints, \
"plingeling_6cores.dat" using 1:7 lt 2 lw 0.5 ps 0.5 title '{/Times=6 manol-pipe-g10nid}' with linespoints, \
"plingeling_6cores.dat" using 1:8 lt 2 lw 0.5 ps 0.5 title '{/Times=6 velev-pipe-uns-1.1-7}' with linespoints

#plot "data.txt" using 2:xticlabels(1) title '' with linespoints #labels in 1
