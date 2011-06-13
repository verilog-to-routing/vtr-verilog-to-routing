	VQM to BLIF Convertor

  Created By:	S. Whitty
		NSERC USRA with Professor Jonathan Rose
		at the University of Toronto

		June 13, 2011

PURPOSE		
------------------------

	The purpose of this project is to convert a circuit netlist in the .vqm format
into the .blif format. This allows for the integration of Altera®'s Quartus® II tool
into other flows that accept .blif as an input format. 

BACKGROUND	
------------------------

	VQM is an internal file format used by Altera inside their development 
tool, Quartus®II. It uses a subset of the Verilog description language, and the 
circuit it describes is technology-mapped to one of their families of devices 
(e.g. Stratix® IV, Cyclone® II, etc.). These files can be dumped using quartus_map and
quartus_cdb, as detailed in Altera's QUIP documentation.
	-> For more, see: quip_tutorial.pdf
	-> Download QUIP at:
	   https://www.altera.com/support/software/download/altera_design/quip/quip-download.jsp?swcode=WWW-SWD-QII-90UIP-ALL

	BLIF is a universal netlist format created at the University of California 
Berkeley. A .blif netlist is characterized by models that describe circuit elements.
Any model has a list of ports (.inputs, .outputs, .clock) and declares flip-flops,
latches and luts. A model can have arbitrary levels of hierarchy, instantiating other
models (declared later in the file) as "subcircuits". If a model is declared as a "blackbox",
there is no knowledge of its inner logic or intraconnect.
	-> For more, see: blif_info.pdf

INSTALL		
------------------------

  1. LIBVQM
  ---------

	libvqm.a contains a VQM parser written by Tomasz Czajkowski (2004).  A standard Makefile
is used for this library, as found in

	/vqm_to_blif/libvqm/

For first-time use, using "make clean" and "make" is recommended. 

NOTE: This compilation requires the use of Bison and Flex, the installation of which is left up
to the user. 

  2. LIBVPR
  ---------

	libvpr_6.a contains an Architecture file (.xml) parser written by Jason Luu (2011). A
standard Makefile is used for this library, as found in

	/vqm_to_blif/libvpr/

For first-time use, using "make clean" and "make" is recommended. 

  3. VQM2BLIF
  -----------

	Finally, a standard Makefile found in 

	/vqm_to_blif/vqm2blif/

	will create the project's executable. For first-time use, typing "make clean" and "make"
is recommended. "make clean" will also remove any of the executable's output files within the working directory.


USAGE
------------------------

	To run the project within the vqm2blif/ directory, enter the following:

	vqm2blif -vqm <VQM File> -arch <ARCH File> 

	Where <VQM File> is the target .vqm file's name [e.g. ../VQM/test.vqm] , and <ARCH File> is the target 
.xml Architecture file containing appropriate elements to place the circuit onto [e.g. ../ARCH/my_arch.xml]. 

	Sample VQM Files can be found in: /vqm_to_blif/VQM/
	Sample ARCH Files can be found in: /vqm_to_blif/ARCH/

  Optional Command-Line Arguments
  -------------------------------

	-out <OUT File>
		-> Specifies a target OUT File, in .blif format [e.g. ../my_test.blif].
		-> If the VPR 6.0 flow is targeted, three output files will be created:
			
			my_test.blif:	
					BLIF netlist containing all connections specified in the 
					VQM, but stripped of constant assignments, which cause 
					errors in the VPR flow. 

			my_test_no_gnd.blif & my_test_no_gnd_no_vcc.blif:
					BLIF netlist, but with "gnd" and "vcc" connections stripped
					and replaced with open ports. This compromises accurate 
					conversion, but allows for testing and verification with
					the current version of VPR. 

		-> Defaults to a filename derived from the input .vqm, located in the working directory.

	-debug
		-> Runtime output is slightly more elaborate. 
		-> Intermediate *.echo files are created with descriptions of the data parsed from the 
		   VQM and XML files (*_module.echo and *_arch.echo, respectively), as well as the 
		   rearranged data prepared for the BLIF file creation (*_blif.echo). 
		-> Defaults to off. 

	-verbose
		-> Runtime output is much more elaborate.
		-> Does nothing if the -debug flag is not set. 
		-> Defaults to off. 

IMPLEMENTATION
------------------------

	Due to the technology-mapped nature of the VQM format, this convertor requires the use of 
VPR's Architecture file format and parser in order to fully describe the netlist in BLIF format. 
For example, BLIF format requires all inputs and outputs of a subblock be explicitly listed as 
unconnected or connected to an external net, while open ports may be omitted in a VQM file. Thus,
the Architecture file contains, among other data, information about all pins available to a subblock,
therefore allowing the convertor to describe the open ports as well. 

	This convertor is designed to be general, meaning that any proper VQM file can be converted
into BLIF as long as an accompanying Architecture file contains instances of the blocks declared in
the VQM. Further updates will allow the elaboration of subblocks into separate components such as LUTs
and Flip-Flops to be declared separately in the BLIF. The Architecture file also supports arbitrary hierarchy,
time-delay information, and routing specifications, among other features. 
	-> For more on the Architecture file syntax, see:
		http://www.eecg.utoronto.ca/vpr/arch_language.html

CONTACT
------------------------

Scott Whitty
whittysc@eecg.utoronto.ca