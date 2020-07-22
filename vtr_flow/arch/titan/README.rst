Titan Benchmarks
--------------------------------------------------
This folder contains architecture files for use with Titan.

The `Titan <http://www.eecg.utoronto.ca/~kmurray/titan/>` benchmarks are distributed
separately from VTR due to their large size.

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
    An enhanced capture of Altera's Stratix IV FPGA architecture. It makes some 
    relatively minor architectural approximations to be compatible with VPR. It
    includes a timing model which has been calibrated to the Stratix IV timing
    model included in Altera's Quartus II CAD tools.
    
    Compared to the original (legacy) capture of Stratix IV, this architecture
    better captures the routing architecture by modeling the hierarchical connections
    between the L16 and L4 networks, and the custom switch patterns between them.
    The L4 wires have 12-input muxes driving them, and the L16 wires have 40:1 muxes
    driving them, roughly matching the Stratix IV mux sizes from the information available.

    This is the architecture used to generate results in the VTR 8.0 TRETS paper,
    and corresponds to architecture E in Table 3 in that paper.

    Use this architecture file, unless you have specific reasons to use the
    other ones in this directory.

stratixiv_arch.timing.complex_sb.12to1.xml:
    A variant of the above architecture capture using customized switch-block
    with hierarchical wire connectivity and fixed 12:1 L4 and L16 driver muxes.

stratixiv_arch.timing.complex_sb.L16_40to1.L4_turn-straight_rand_L4_L16.xml:
    A variant like stratixiv_arch.timing.complex_sb.12to1.xml, but increased the
    L16 driver muxes to 40:1.

stratixiv_arch.timing.complex_sb.L16_72to1.L4_turn-straight_rand_L4_L16.xml:
    A variant like stratixiv_arch.timing.complex_sb.12to1.xml, but increased the
    L16 driver muxes to 72:1.

stratixiv_arch.timing.complex_sb.L4_16to1.L16_72to1.L4_turn-straight_rand_L4_L16.xml:
    A variant like stratixiv_arch.timing.complex_sb.12to1.xml, but increased the
    L4 driver muxes to 16:1 and the L16 driver muxes to 72:1.

For more details about the Altera's Stratix IV FPGA architecture capture variants,
see Section 5.4 of the paper, "VTR 8: High Performance CAD and Customizable FPGA Architecture Modelling": 
<https://www.eecg.utoronto.ca/~kmurray/vtr/vtr8_trets.pdf>

**legacy subdirectory:**

    stratixiv_arch.timing.legacy.xml:
        The old capture of Altera's Stratix IV FPGA architecture.
        This architecture was used to generate the results in the Timing-driven Titan paper:
        K. Murray et al, "Timing-Driven Titan: Enabling Large Benchmarks and Exploring the
        Gap between Academic and Commercial CAD," ACM TRETS, March 2015,
        <https://dl.acm.org/doi/10.1145/2629579>

    stratixiv_arch.timing.no_chain.xml (experimental):
        Like stratixiv_arch.timing.xml, but with carry chains disabled.

    stratixiv_arch.timing.no_directlink.xml (experimental):
        Like stratixiv_arch.timing.xml, but with direct-links disabled.

    stratixiv_arch.timing.no_pack_patterns.xml (experimental):
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
