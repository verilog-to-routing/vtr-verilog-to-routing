kmult: Automatic Addition Chain Generator
=========================================
The purpose of this program is to generate a multiplierless circuit,
that can multiply by a certain set of constants. Given a set of
constants it first generates single-constant-multiplication circuits
of minimum area, which are then fused into a time-multiplexing
circuit, which can multiply by all constants, but only one at a
time. The constants in the time-multiplexing circuit can be selected
over a control-input.

Additionally a graphical representation of the circuit is
generated. It can be visualized using GraphViz, a graphical
visualisation software (http://www.graphviz.org/)

The program was written by Peter Tummeltshammer while working in
the Spiral project (www.spiral.net). The program is based on the following papers:

Peter Tummeltshammer, James C. Hoe and Markus Püschel
Time-Multiplexed Multiple Constant Multiplication
IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, Vol. 26, No. 9, pp. 1551-1563, 2007

Peter Tummeltshammer, James C. Hoe and Markus Püschel
Multiple Constant Multiplication By Time-Multiplexed Mapping of Addition Chains
Proc. Design Automation Conference (DAC), pp. 826-829, 2004 

Installation:
just compile with "make"


Synopsys: 
./kmult [-g NumberIterations -n NumberConstants] [-r
RandomSearchSize] [-t Table-file] [-o Verilog-outputfile] [-d
Graphviz-outputfile] [-i Inputbitwidth] [-c Constantbitwidth] [-f
Fractionbitwidth] [-v] [-O Optimization] constant1 constant2
... constantN

Description:

  -g NumberIterations -n NumberConstants
    performs <NumberIterations> fusions with <NumberConstants>
    random constants for each fusion. Using this option, no constants
    have to be specified. The average area cost is given in the end.

  -r RandomSearchSize
    specifies the search size for each fusion (number of
    reorderings). If -r is not selected, the best ordering is
    searched for exhaustively, so RandomSearchSize should be smaller
    than NumberConstants!.

  -t Table-file
    the single-constant-multiplication circuits can be read from a
    table-file.

  -o Verilog-outputfile
    specifies the outputpath and filename for the Verilog-output. If
    not specified, the default is ./module.v 

  -d Graphviz-outputfile
    specifies the outputpath and filename for the graphviz-output. If
    not specified, the default is ./module.dot 

  -i Inputbitwidth 
    specifies the bitwidth of the data input. Default is 16

  -c Constantbitwidth 
    specifies the bitwidth for the constants. When using option -g the
    randomly drawn constants will always be in
    [1..2^Constantbitwidth-1]. Default is 8.

  -f Fractionbitwidth
    specifies the fractional bitwidth for the constants. The circuit's
    output bitwith will therefor be Inputbitwidth + Constantbitwidth -
    Fractionbitwidth. Default is 8

  -v
    Verbose

  -O Optimization
    by setting Optimization != 0 area optimization is performed
    during the finding of the single-constant-multiplication circuits,
    by finding fundamental overlappings. -O can only be used without
    -t.  WARNING: this option is still under test, and might deliver
    bad results. Default is 0.



Examples:

./kmult 25 27 29 
fuses these three constants exhaustively and generates the files
module.v and module.dot

./kmult -g100 -n8 -r1000 -c10 -t21-no-shr.chains
randomly generates 100 sets of 8 10-bit constants, and fuses them with a
RandomSearchSize of 1000 (1000 reorderings for each fusion). The
single-constant-multiplication circuits are taken from the Table-file
21-no-shr.chains.
