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
set output "experiment1.eps"
set border
set size square 0.5,0.5 # percent of original (50% of its size here)
unset time # unset for no time
set lmargin 10
set bmargin -1
set rmargin 2
set tmargin 1

# Details for this particular plot
set key bottom # legend to the left (bottom is also an option)
unset logscale y  #set for logarithmic scale y (add x for log x)
set xlabel "# of threads" # XAxis title
set ylabel "Speedup" # YAxis title
set key at -0.5,0.2
set key left
set key spacing 0.6
set border 3 lw 0.5
set xtics nomirror
set ytics nomirror
plot "experiment1.dat" using 1:2 lt 2 lw 0.5 ps 0.5 title '{/Times=6 anbul-dated-5-15-u.cnf}' with linespoints, \
"experiment1.dat" using 1:3 lt 2 lw 0.5 ps 0.5 title '{/Times=6 anbul-part-10-15-s.cnf}' with linespoints, \
"experiment1.dat" using 1:4 lt 2 lw 0.5 ps 0.5 title '{/Times=6 fuhs-aprove-16.cnf}' with linespoints, \
"experiment1.dat" using 1:5 lt 2 lw 0.5 ps 0.5 title '{/Times=6 goldb-heqc-alu4mul.cnf}' with linespoints, \
"experiment1.dat" using 1:6 lt 2 lw 0.5 ps 0.5 title '{/Times=6 goldb-heqc-i8mul.cnf}' with linespoints, \
"experiment1.dat" using 1:7 lt 2 lw 0.5 ps 0.5 title '{/Times=6 grieu-vmpc-s05-27r.cnf}' with linespoints, \
"experiment1.dat" using 1:8 lt 2 lw 0.5 ps 0.5 title '{/Times=6 ibm-2002-20r-k75.cnf}' with linespoints, \
"experiment1.dat" using 1:9 lt 2 lw 0.5 ps 0.5 title '{/Times=6 ibm-2002-24r3-k100.cnf}' with linespoints, \
"experiment1.dat" using 1:10 lt 2 lw 0.5 ps 0.5 title '{/Times=6 ibm-2002-31_1r3-k30.cnf}' with linespoints, \
"experiment1.dat" using 1:11 lt 2 lw 0.5 ps 0.5 title '{/Times=6 ibm-2004-23-k80.cnf}' with linespoints, \
"experiment1.dat" using 1:12 lt 2 lw 0.5 ps 0.5 title '{/Times=6 manol-pipe-c7idw.cnf}' with linespoints, \
"experiment1.dat" using 1:13 lt 2 lw 0.5 ps 0.5 title '{/Times=6 manol-pipe-g10bid_i.cnf}' with linespoints, \
"experiment1.dat" using 1:14 lt 2 lw 0.5 ps 0.5 title '{/Times=6 mizh-md5-47-4.cnf}' with linespoints, \
"experiment1.dat" using 1:15 lt 2 lw 0.5 ps 0.5 title '{/Times=6 mizh-sha0-36-4.cnf}' with linespoints, \
"experiment1.dat" using 1:16 lt 2 lw 0.5 ps 0.5 title '{/Times=6 narai-vpn-10s.cnf}' with linespoints, \
"experiment1.dat" using 1:17 lt 2 lw 0.5 ps 0.5 title '{/Times=6 post-c32s-gcdm16-22.cnf}' with linespoints, \
"experiment1.dat" using 1:18 lt 2 lw 0.5 ps 0.5 title '{/Times=6 schup-l2s-motst-2-k315.cnf}' with linespoints, \
"experiment1.dat" using 1:19 lt 2 lw 0.5 ps 0.5 title '{/Times=6 simon-s02b-r4b1k1.2.cnf}' with linespoints, \
"experiment1.dat" using 1:20 lt 2 lw 0.5 ps 0.5 title '{/Times=6 simon-s02-f2clk-50.cnf}' with linespoints, \
"experiment1.dat" using 1:21 lt 2 lw 0.5 ps 0.5 title '{/Times=6 velev-npe-1.0-9dlx-b71.cnf}' with linespoints

#plot "data.txt" using 2:xticlabels(1) title '' with linespoints #labels in 1
