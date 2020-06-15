This folder contains architecture files for use with Titan.

The `Titan <http://www.eecg.utoronto.ca/~kmurray/titan/>` benchmarks and 
architectures are distributed separately from VTR due to their large size.

To integrate them into VTR run:

    $ make get_titan_benchmarks

from the root of the VTR source tree.

This will download and extract the benchmark netlists to:

    <vtr>/vtr_flow/benchmarks/titan_blif/

and

    <vtr>/vtr_flow/benchmarks/titan_other_blif/

where 'titan_blif' contains the main Titan23 benchmarks, and 'titan_other_blif' contains smaller 
titan-like benchmarks which are useful for testing (but should not be used for architecture and 
CAD evaluation).


Directory Structure
--------------------------------------------------
stratixiv_arch.timing.xml:
    A capture of Altera's Stratix IV FPGA architecture. It makes some 
    relatively minor architectural approximations to be compatible with
    VPR.  It includes a timing model which has been calibrated to the
    Stratix IV timing model included in Altera's Quartus II CAD tools.

    Use this architecture file, unless you have specific reasons to use 
    stratixiv_arch.detailed.xml or stratixiv_arch.simple.xml.

stratixiv_arch.timing.no_chain.xml:
    Like stratixiv_arch.timing.xml, but with carry chains disabled.

stratixiv_arch.timing.no_directlink.xml:
    Like stratixiv_arch.timing.xml, but with direct-links disabled.

stratixiv_arch.timing.no_pack_patterns.xml:
    Like stratixiv_arch.timing.xml, but with DSP pack patterns disabled.
    
    
Modifying Architecture Files
--------------------------------------------------
    Due the length of these architecture files, it may be useful to use a text 
    editor which support "folding" of XML tags (e.g. vim).  This can greatly
    simplify the editing process, allowing you to focus only on the sections of
    interest.

    Most parts of the Architecture can be modified by hand. However care must
    be taken when changing some fields (e.g. a <pb_type>'s num_pb) to ensure
    that the the interconnect between pb_types remains correct.

    If you wish to modify the memory architecture (i.e. RAM blocks), it is 
    probably best to use gen_stratixiv_memories_class.py, rather than attempt 
    to do so by hand because of the large number of possible operating modes.

Adding Support for New Architectures
--------------------------------------------------
    Support can be added for additional Quartus II supported FPGA architectures 
    (Cyclone III, Stratix II etc), by defining models for the architecture's VQM
    primitives.  Good places to look for this information include:
       * Altera's Quartus Univeristy Interface Program (QUIP) documentation
       * The 'fv_lib' directory under a Quartus installation

    For more details see vqm_to_blif's README.txt
