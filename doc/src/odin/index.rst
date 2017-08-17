.. _odin_II:
#############
Odin II
#############

Odin II is used for logic synthesis and elaboration, converting a subset of the Verilog Hardware Description Language (HDL) into a BLIF netlist.
		
.. seealso:: :cite:`jamieson_odin_II`

*************
USAGE
*************
===========
Required
===========

.. list-table::

 * - ``-c``
   - <XML Configuration File>
 * - ``-V``
   - <Verilog HDL File>
 * - ``-b``
   - <BLIF File>
   
   
===========   
Options
===========

.. list-table::

 * - ``-o``
   - <output file>
   - full output path and file name
 * - ``-a``
   - <architecture file>
   - architecture file in VPR format
 * - ``-G``
   - 
   - Output netlist graph in graphviz .dot format. (net.dot, opens with dotty)
 * - ``-A``
   - 
   - Output AST graph in .dot format.
 * - ``-W``
   - 
   - Print all warnings. (Can be substantial.) 
 * - ``-h``
   - 
   - Print help


===========
Simulation
===========
    

Activate Simulation with either
************  

.. list-table::

 * - ``-g``
   - <number>
   - Number of random test vectors to generate
 * - ``-L``
   - <Comma-separated list>
   - Comma-separated list of primary inputs to hold high at cycle 0, and low for all subsequent cycles.
 * - ``-3``
   - 
   - Generate three valued logic. (Default is binary.)
 * - ``-t``
   - <input vector file>
   - Supply a predefined input vector fil
 * - ``-U0``
   - 
   - initial register value to 0
 * - ``-U1``
   - 
   - initial register value to 1 
 * - ``-UX``
   - 
   - initial register value to X(unknown) (DEFAULT)
    
Options 
************

.. list-table::

 * - ``-T``
   - <output vector file>
   - Supply an output vector file to check output vectors against
 * - ``-E``
   - 
   - Output after both edges of the clock. (Default is to output only after the falling edge.)
 * - ``-R``
   - 
   - Output after rising edge of the clock only. (Default is to output only after the falling edge.)
 * - ``-p``
   - <Comma-separated list>
   - Comma-separated list of additional pins/nodes to monitor during simulation. (view NOTES)

Example for ``-p``:
####

.. list-table::

 * - ``-p input~0,input~1``
   - monitors pin 0 and 1 of input
 * - ``-p input``
   - monitors all pins of input as a single port
 * - ``-p input~``
   - monitors all pins of input as separate ports. (split)
   
.. note::

	Matching for ``-p'' is done via strstr so general strings will match all similar pins and nodes. (Eg: FF_NODE will create a single port with all flipflops) 

.. note::

	Simulation always produces files:

	    - input_vectors 
	    - output_vectors
	    - test.do (ModelSim)
    
===========                
Verilog Supported Idioms:
===========

Operators:

- **
- ||
- &&
- <=
- =>
- >=
- <<
- <<<
- >>
- ==
- !=
- ===
- !==
- ^~
- ~^
- ~&
- ~|


Keyword	:	

- always  
- and    
- assign 
- begin  
- case   
- default
- `define
- defparam	
- else
- end	
- endcase	
- endfunction
- endmodule	
- endspecify
- if	
- initial	
- inout	
- input	
- integer
- module
- function
- nand	
- negedge	
- nor
- not	
- or	
- output
- parameter
- localparam	
- posedge
- reg	
- specify	
- wire	
- xnor	
- xor	
- @()	
- @*	

*************
INSTALL
*************

===========
Prerequisites
===========

1. ctags
2. bison
3. flex
4. gcc 5.x
5. cmake 2.8.12 (minimum version)
6. time 
7. cairo

===========
INSTALL
===========

To build ODIN, run "make odin_II" from the vtr root directory.

.. note::
	ODIN uses CMake as it's build system. CMake provides a protable cross-platform build systems with many useful features.
	For unix-like systems we provide a wrapper Makefile which supports the traditional make and make clean commands,
	but calls CMake behind the scenes.

.. warning::
	After you build Odin, please run the included verify_microbenchmarks.sh script. This will automatically compile, simulate, 
	and verify all of the included microbenchmark circuits to ensure that Odin is working correctly on your system. 

