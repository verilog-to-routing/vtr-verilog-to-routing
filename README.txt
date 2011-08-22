		-------------------------------------
			VQM to BLIF Convertor
		-------------------------------------

  Created By:	S. Whitty
		NSERC USRA with Professor Jonathan Rose and Professor Vaughn Betz
		The Edward S. Rogers Sr. Department of Electrical and Computer Engineering
		University of Toronto

		August 22, 2011

--------------------------------------
	I. 	PURPOSE
	II. 	BACKGROUND
	III. 	THEORY
	IV. 	INSTALLATION
	V.	SAMPLE USAGE
	VI. 	GENERAL USAGE
	VII.	VQM GENERATION
	IIX. 	CONTACT
--------------------------------------

I. PURPOSE		
------------------------

	The purpose of this project is to convert a circuit netlist in the VQM format into the BLIF format.
This allows for the integration of Altera®'s Quartus® II tool into other flows that accept BLIF as an input 
format. 

II. BACKGROUND	
------------------------

	VQM is an internal file format used by Altera inside their development tool, Quartus®II. It uses a 
subset of the Verilog description language, and the circuit it describes is technology-mapped to one of 
their families of devices (e.g. Stratix® IV, Cyclone® II, etc.). These files can be generated using 
quartus_map and quartus_cdb, as detailed below (See: VII. VQM GENERATION), or in Altera's QUIP documentation.
	-> For more on QUIP, see: /vqm_to_blif/DOCS/quip_tutorial.pdf
	-> Download QUIP at:
	   https://www.altera.com/support/software/download/altera_design/quip/quip-download.jsp?swcode=WWW-SWD-QII-90UIP-ALL

	BLIF is a universal netlist format created at the University of California Berkeley. A BLIF netlist 
is characterized by models that describe circuit elements. Any model has a list of ports (.inputs, .outputs) 
and declares flip-flops, latches and luts. A model can have arbitrary levels of hierarchy, instantiating other
models (declared later in the file) as "subcircuits". If a model is declared as a "blackbox", there is no 
knowledge or possible optimization of its inner logic or intraconnect.
	-> For more, see: /vqm_to_blif/DOCS/blif_info.pdf


III. THEORY
------------------------

	The VQM to BLIF translator requires two mandatory inputs; a VQM file to be translated, and an 
Architecture file describing the primitives that appear in the VQM. The Architecture is necessary based on 
the discrepancies between VQM and BLIF, predominantly the lack of open-port specification in the VQM syntax. 
	
	In the BLIF standard, each subcircuit (complex block) instantiation requires a model to be 
associated with it. These models appear at the bottom of the BLIFs, and exhaustively list all the ports 
available on the block pin-by-pin. Each instantiation must, in turn, exhaustively list the connectivity of 
the block. In the interest of adhering to the standard set in VTR, an open input port on a subcircuit is 
connected to a dummy net called "unconn" and an open output port has a dummy net derived for it from its 
name and the block's instantiation number (e.g. "subckt2~portA~unconn"). 

	In the VQM format, however, only the connected ports of a subcircuit module are listed; all 
unconnected ports are left unspecified. In order to represent blocks of the same type but with varying levels
of connectivity, the Architecture file is introduced. Since it exhaustively lists the ports available to each 
block type, any port listed in the Architecture file but not in the VQM can be inferred by the translator as 
being "open." The Architecture file format was chosen for its potential use in a downstream VPR-processing of 
the BLIF.

	-> For more on the Architecture file syntax, see:
		http://www.eecg.utoronto.ca/vpr/arch_language.html


IV. INSTALLATION	
------------------------

	Within the root directory /vqm_to_blif/ type "make". This will compile all source code
and create statically-linked libraries in /vqm_to_blif/SRC/include/libvpr and /vqm_to_blif/SRC/include/libvqm
that the project requires.

NOTE: This compilation requires the use of Bison and Flex, the installation of which is left up to the user. 

	-> libvqm.a contains a VQM parser written by Tomasz Czajkowski (2004).
	-> libvpr_6.a contains an Architecture file (XML) parser written by Jason Luu (2011).


V. SAMPLE USAGE
------------------------
	
	After installation, run the following commands in the /vqm_to_blif/ directory to ensure 
proper functionality:

	vqm2blif -vqm VQM/simple_compare_stratixiv.vqm -arch ARCH/stratixiv_arch.xml -luts vqm -elab none -clean all -buffouts -out simple_compare_test.blif
	diff simple_compare_test.blif BLIF/simple_compare_golden.blif

VI. GENERAL USAGE
------------------------

	To run the translator, enter the following command:

	vqm2blif -vqm <VQM File>.vqm -arch <ARCH File>.xml

Where <VQM File> is the target VQM file's name [e.g. ../VQM/test.vqm] , and <ARCH File> is the target XML 
Architecture file containing the constinuent primitives of the circuit [e.g. ../ARCH/my_arch.xml]. 

	-> Sample VQM Files can be found in: /vqm_to_blif/VQM/
	-> Sample ARCH Files can be found in: /vqm_to_blif/ARCH/

  Optional Command-Line Arguments
  -------------------------------
	[Note: All command-line arguments, including the mandatory ones above, are order-independant.]

	-out <OUT File>
		Specifies a target BLIF file to output the translation to [e.g. ../my_test.blif].
		-> Defaults to a filename derived from the input .vqm, located in the working directory.

	-elab [none | modes]
		Instructs the tool on how to interpret VQM modules as BLIF primitives.
		-> No Elaboration: 
			Parameter information of VQM modules is completely ignored. Primitives in the BLIF
		   are named based solely on the name of each module. Note that this will result in loss of
		   functional information.
		-> Mode-Elaboration (default): 
			Derives an operation mode for each primitive and appends it to its name. This allows
		   for finer-grained information (e.g. timing) to be determined based on the VQM primitives' 
		   configurations. Currently, the mode name is generated from the block's "operation_mode" 
		   parameter as follows: 

			"<Block Name>.opmode{<operation_mode Value>}"
			-> e.g. "stratixiv_mac_out", operation_mode="output_only" ==> "stratixiv_mac_out.opmode{output_only}"

		   If a block does not have a parameter called "operation_mode", the mode-appended name is
		   simply the Block Name, as in No Elaboration.

		Note:	The architecture file must contain primitives corresponding to each primitive 
			possible in the BLIF. If using Mode-Elaboration, each possible string value of
			operation_mode must be known and appear with a  unique primitive within the 
			architecture. 

	-clean [none | buffers | all]
		Instructs the tool on whether to remove buffers and/or invertors introduced in the VQM File.
		Many primitives in the VQM have a programmable inversion feature on their inputs that 
		is expressed through the instantiation of an invertor. It can be assumed that all invertors 
		located immediately in front of a blackbox-primitive can be absorbed into the primitive.
		-> No Cleaning: 
			Skips all cleaning steps, does not create a netlist structure.
		-> Buffer Cleaning: 
			Creates a netlist structure and removes all the buffers it can within the circuit.
		-> Total Cleaning (default): 
			Creates a netlist structure, removes all buffers and absorbs all invertors that it 
		   can while still conforming to BLIF standards.

	-buffouts
		Regardless of the Clean setting, turning this flag on keeps buffers resulting in an 
		output-pin sink. This is sometimes necessary for downstream logic synthesis tools (e.g. ABC).
		-> Defaults to off.


	-luts [vqm | blif]
		Instructs the tool on how to interpret LUTs that appear in the VQM into BLIF. 
		-> VQM Mode: 
			LUTs in the VQM file appear as blackboxes in the BLIF with the same name.
		-> BLIF Mode (default): 
			Elaborates LUTs in the VQM file into corresponding ".names" structures in the BLIF. 

		Notes: 
			1. The current version can only properly identify specifically-configured 
			   Stratix IV-Family LCELL blocks as LUTs to be elaborated.
		   	2. Invertors in front of elaborated LUTs are never absorbed. 
			3. Output may repeat rows of a LUT's truth table, but this does not impact the 
			   legality or functionality of the BLIF. 
		   	4. A downstream logic synthesis tool (e.g. ABC) can remove the row rendundancy and 
			   absorb invertors into the logic. 

	-debug
		Intermediate *.echo files are created with descriptions of the data parsed from the VQM and
		XML files (*_module.echo and *_arch.echo, respectively), as well as the rearranged data 
		prepared for the BLIF file creation (*_blif.echo). 
		-> Defaults to off. 

	-verbose
		Runtime console output is much more elaborate.
		-> Defaults to off. 

VII. VQM GENERATION
------------------------

	This method uses a Linux Shell environment to generate a VQM file using Quartus II. Other methods, 
such as using a Windows environment or the Quartus II GUI, are not covered here.

	Before beginning, ensure you have located the following:
		1. The Verilog file of the circuit, compatible with Altera's Quartus II Software
		2. The quartus_map and quartus_cdb executables from the Quartus II Installation

	----------
	  Step 1
	----------
	Execute the following within the directory of the verilog file:
		
		quartus_map <Verilog File>.v --family="<Intended Device Family>"

	This will generate the compilation files required to generate the VQM.

	------------------------------
	  Step 1.5 (Stratix IV only)
	------------------------------
	If Stratix IV is the intended device family, VQM output is not supported by default. After executing
	quartus_map and generating the appropriate Quartus files, locate the .qsf file associated with your 
	project. Open it, then insert the following line at the end of the file:

		set_global_assignment -name INI_VARS "qatm_force_vqm=on;vqmo_gen_sivgx_vqm=on"

	After this, re-run quartus_map as in Step 1.

	Note: This step can also be performed using TCL commands.

	----------
	  Step 2
	----------
	Execute the following:
		
		quartus_cdb <Project Name> --vqm="<VQM File>.vqm"

	And the VQM file is generated!

	Some projects that include proprietary information (e.g. some Altera MegaWizard modules) will result 
in an encrypted VQM File. Once the VQM is generated, make sure to open it and inspect the result; it should 
appear very similar to Verilog HDL. If not, go back to your Quartus II project, then locate and remove the 
proprietary modules. If a Verilog file is included with your project that causes the encryption, it must be 
removed from the project to allow proper VQM output.

For a more detailed description of VQM generation, see /vqm_to_blif/DOCS/quip_tutorial.pdf or download
the QUIP package from Altera at:
	-> https://www.altera.com/support/software/download/altera_design/quip/quip-download.jsp?swcode=WWW-SWD-QII-90UIP-ALL

	
IIX. CONTACT
------------------------

Scott Whitty
whittysc@eecg.utoronto.ca