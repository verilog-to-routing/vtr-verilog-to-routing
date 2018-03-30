.. _timing_simulation_tutorial:

Post-Implementation Timing Simulation
-------------------------------------

.. _fig_timing_simulation:

.. figure:: timing_simulation.*

    Timing simulation waveform for ``stereovision3``

This tutorial describes how to simulate a circuit which has been implemented by :ref:`VPR` with back-annotated timing delays.

Back-annotated timing simulation is useful for a variety of reasons:
 * Checking that the circuit logic is correctly implemented
 * Checking that the circuit behaves correctly at speed with realistic delays
 * Generating VCD (Value Change Dump) files with realistic delays (e.g. for power estimation)


Generating the Post-Implementation Netlist
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
For the purposes of this tutorial we will be using the ``stereovision3`` :ref:`benchmark <benchmarks>`, and will target the ``k6_N10_40nm`` architecture.

First lets create a directory to work in:

.. code-block:: console

    $ mkdir timing_sim_tut
    $ cd timing_sim_tut

Next we'll copy over the ``stereovision3`` benchmark netlist in BLIF format and the FPGA architecture description:

.. code-block:: console

    $ cp $VTR_ROOT/vtr_flow/benchmarks/vtr_benchmarks_blif/stereovision3.blif .
    $ cp $VTR_ROOT/vtr_flow/arch/timing/k6_N10_40nm.xml .

.. note:: Replace :term:`$VTR_ROOT` with the root directory of the VTR source tree

Now we can run VPR to implement the circuit onto the ``k6_N10_40nm`` architecture.
We also need to provide the :option:`vpr --gen_post_synthesis_netlist` option to generate the post-implementation netlist and dump the timing information in Standard Delay Format (SDF)::

    $ vpr k6_N10_40nm.xml stereovision3.blif --gen_post_synthesis_netlist on

Once VPR has completed we should see the generated verilog netlist and SDF:

.. code-block:: console

    $ ls *.v *.sdf
    sv_chip3_hierarchy_no_mem_post_synthesis.sdf  sv_chip3_hierarchy_no_mem_post_synthesis.v


Inspecting the Post-Implementation Netlist
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Lets take a quick look at the generated files.

First is a snippet of the verilog netlist:

.. code-block:: verilog
    :caption: Verilog netlist snippet
    :name: post_imp_verilog
    :emphasize-lines: 1,8,22

    fpga_interconnect \routing_segment_lut_n616_output_0_0_to_lut_n497_input_0_4  (
        .datain(\lut_n616_output_0_0 ),
        .dataout(\lut_n497_input_0_4 )
    );


    //Cell instances
    LUT_K #(
        .K(6),
        .LUT_MASK(64'b0000000000000000000000000000000000100001001000100000000100000010)
    ) \lut_n452  (
        .in({
            1'b0,
            \lut_n452_input_0_4 ,
            \lut_n452_input_0_3 ,
            \lut_n452_input_0_2 ,
            1'b0,
            \lut_n452_input_0_0 }),
        .out(\lut_n452_output_0_0 )
    );

    DFF #(
        .INITIAL_VALUE(1'b0)
    ) \latch_top^FF_NODE~387  (
        .D(\latch_top^FF_NODE~387_input_0_0 ),
        .Q(\latch_top^FF_NODE~387_output_0_0 ),
        .clock(\latch_top^FF_NODE~387_clock_0_0 )
    );

Here we see three primitives instantiated:

* ``fpga_interconnect`` represent connections between netlist primitives
* ``LUT_K`` represent look-up tables (LUTs) (corresponding to ``.names`` in the BLIF netlist). Two parameters define the LUTs functionality:

     * ``K`` the number of inputs, and
     * ``LUT_MASK`` which defines the logic function.

* ``DFF`` represents a D-Flip-Flop (corresponding to ``.latch`` in the BLIF netlist).

    * The ``INITIAL_VALUE`` parameter defines the Flip-Flop's initial state.

Different circuits may produce other types of netlist primitives corresponding to hardened primitive blocks in the FPGA such as adders, multipliers and single or dual port RAM blocks.

.. note:: The different primitives produced by VPR are defined in ``$VTR_ROOT/vtr_flow/primitives.v``


Lets now take a look at the Standard Delay Fromat (SDF) file:

.. code-block:: none
    :linenos:
    :caption: SDF snippet
    :name: post_imp_sdf_
    :emphasize-lines: 2-3,12-13,25-26

    (CELL
        (CELLTYPE "fpga_interconnect")
        (INSTANCE routing_segment_lut_n616_output_0_0_to_lut_n497_input_0_4)
        (DELAY
            (ABSOLUTE
                (IOPATH datain dataout (312.648:312.648:312.648) (312.648:312.648:312.648))
            )
        )
    )

    (CELL
        (CELLTYPE "LUT_K")
        (INSTANCE lut_n452)
        (DELAY
            (ABSOLUTE
                (IOPATH in[0] out (261:261:261) (261:261:261))
                (IOPATH in[2] out (261:261:261) (261:261:261))
                (IOPATH in[3] out (261:261:261) (261:261:261))
                (IOPATH in[4] out (261:261:261) (261:261:261))
            )
        )
    )

    (CELL
        (CELLTYPE "DFF")
        (INSTANCE latch_top\^FF_NODE\~387)
        (DELAY
            (ABSOLUTE
                (IOPATH (posedge clock) Q (124:124:124) (124:124:124))
            )
        )
        (TIMINGCHECK
            (SETUP D (posedge clock) (66:66:66))
        )
    )

The SDF defines all the delays in the circuit using the delays calculated by VPR's STA engine from the architecture file we provided.

Here we see the timing description of the cells in :numref:`post_imp_verilog`.

In this case the routing segment ``routing_segment_lut_n616_output_0_0_to_lut_n497_input_0_4`` has a delay of 312.648 ps, while the LUT ``lut_n452`` has a delay of 261 ps from each input to the output.
The DFF ``latch_top\^FF_NODE\~387`` has a clock-to-q delay of 124 ps and a setup time of 66ps.

Creating a Test Bench
~~~~~~~~~~~~~~~~~~~~~
In order to simulate a benchmark we need a test bench which will stimulate our circuit (the Device-Under-Test or DUT).

An example test bench which will randomly perturb the inputs is shown below:

.. code-block:: systemverilog
    :linenos:
    :caption: The test bench ``tb.sv``.
    :emphasize-lines: 69,72,75-76

    `timescale 1ps/1ps
    module tb();

    localparam CLOCK_PERIOD = 8000;
    localparam CLOCK_DELAY = CLOCK_PERIOD / 2;

    //Simulation clock
    logic sim_clk;

    //DUT inputs
    logic \top^tm3_clk_v0 ;
    logic \top^tm3_clk_v2 ;
    logic \top^tm3_vidin_llc ;
    logic \top^tm3_vidin_vs ;
    logic \top^tm3_vidin_href ;
    logic \top^tm3_vidin_cref ;
    logic \top^tm3_vidin_rts0 ;
    logic \top^tm3_vidin_vpo~0 ;
    logic \top^tm3_vidin_vpo~1 ;
    logic \top^tm3_vidin_vpo~2 ;
    logic \top^tm3_vidin_vpo~3 ;
    logic \top^tm3_vidin_vpo~4 ;
    logic \top^tm3_vidin_vpo~5 ;
    logic \top^tm3_vidin_vpo~6 ;
    logic \top^tm3_vidin_vpo~7 ;
    logic \top^tm3_vidin_vpo~8 ;
    logic \top^tm3_vidin_vpo~9 ;
    logic \top^tm3_vidin_vpo~10 ;
    logic \top^tm3_vidin_vpo~11 ;
    logic \top^tm3_vidin_vpo~12 ;
    logic \top^tm3_vidin_vpo~13 ;
    logic \top^tm3_vidin_vpo~14 ;
    logic \top^tm3_vidin_vpo~15 ;

    //DUT outputs
    logic \top^tm3_vidin_sda ;
    logic \top^tm3_vidin_scl ;
    logic \top^vidin_new_data ;
    logic \top^vidin_rgb_reg~0 ;
    logic \top^vidin_rgb_reg~1 ;
    logic \top^vidin_rgb_reg~2 ;
    logic \top^vidin_rgb_reg~3 ;
    logic \top^vidin_rgb_reg~4 ;
    logic \top^vidin_rgb_reg~5 ;
    logic \top^vidin_rgb_reg~6 ;
    logic \top^vidin_rgb_reg~7 ;
    logic \top^vidin_addr_reg~0 ;
    logic \top^vidin_addr_reg~1 ;
    logic \top^vidin_addr_reg~2 ;
    logic \top^vidin_addr_reg~3 ;
    logic \top^vidin_addr_reg~4 ;
    logic \top^vidin_addr_reg~5 ;
    logic \top^vidin_addr_reg~6 ;
    logic \top^vidin_addr_reg~7 ;
    logic \top^vidin_addr_reg~8 ;
    logic \top^vidin_addr_reg~9 ;
    logic \top^vidin_addr_reg~10 ;
    logic \top^vidin_addr_reg~11 ;
    logic \top^vidin_addr_reg~12 ;
    logic \top^vidin_addr_reg~13 ;
    logic \top^vidin_addr_reg~14 ;
    logic \top^vidin_addr_reg~15 ;
    logic \top^vidin_addr_reg~16 ;
    logic \top^vidin_addr_reg~17 ;
    logic \top^vidin_addr_reg~18 ;


    //Instantiate the dut
    sv_chip3_hierarchy_no_mem dut ( .* );

    //Load the SDF
    initial $sdf_annotate("sv_chip3_hierarchy_no_mem_post_synthesis.sdf", dut);

    //The simulation clock
    initial sim_clk = '1;
    always #CLOCK_DELAY sim_clk = ~sim_clk;

    //The circuit clocks
    assign \top^tm3_clk_v0 = sim_clk;
    assign \top^tm3_clk_v2 = sim_clk;

    //Randomized input
    always@(posedge sim_clk) begin
        \top^tm3_vidin_llc <= $urandom_range(1,0);
        \top^tm3_vidin_vs <= $urandom_range(1,0);
        \top^tm3_vidin_href <= $urandom_range(1,0);
        \top^tm3_vidin_cref <= $urandom_range(1,0);
        \top^tm3_vidin_rts0 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~0 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~1 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~2 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~3 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~4 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~5 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~6 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~7 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~8 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~9 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~10 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~11 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~12 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~13 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~14 <= $urandom_range(1,0);
        \top^tm3_vidin_vpo~15 <= $urandom_range(1,0);
    end

    endmodule

The testbench instantiates our circuit as ``dut`` at line 69.
To load the SDF we use the ``$sdf_annotate()`` system task (line 72) passing the SDF filename and target instance.
The clock is defined on lines 75-76 and the random circuit inputs are generated at the rising edge of the clock on lines 84-104.

Performing Timing Simulation in Modelsim
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
To perform the timing simulation we will use *Modelsim*, an HDL simulator from Mentor Graphics.

.. note:: Other simulators may use different commands, but the general approach will be similar.

It is easiest to write a ``tb.do`` file to setup and configure the simulation:

.. code-block:: tcl
    :linenos:
    :caption: Modelsim do file ``tb.do``. Note that :term:`$VTR_ROOT` should be replaced with the relevant path.
    :emphasize-lines: 12-14,17,31

    #Enable command logging
    transcript on

    #Setup working directories
    if {[file exists gate_work]} {
        vdel -lib gate_work -all
    }
    vlib gate_work
    vmap work gate_work

    #Load the verilog files
    vlog -sv -work work {sv_chip3_hierarchy_no_mem_post_synthesis.v}
    vlog -sv -work work {tb.sv}
    vlog -sv -work work {$VTR_ROOT/vtr_flow/primitives.v}

    #Setup the simulation
    vsim -t 1ps -L gate_work -L work -voptargs="+acc" +sdf_verbose +bitblast tb

    #Log signal changes to a VCD file
    vcd file sim.vcd
    vcd add /tb/dut/*
    vcd add /tb/dut/*

    #Setup the waveform viewer
    log -r /tb/*
    add wave /tb/*
    view structure
    view signals

    #Run the simulation for 1 microsecond
    run 1us -all

We link together the post-implementation netlist, test bench and VTR primitives on lines 12-14.
The simulation is then configured on line 17, some of the options are worth discussing in more detail:

* ``+bitblast``: Ensures Modelsim interprets the primitives in ``primitives.v`` correctly for SDF back-annotation.

.. warning:: Failing to provide ``+bitblast`` can cause errors during SDF back-annotation

* ``+sdf_verbose``: Produces more information about SDF back-annotation, useful for verifying that back-annotation succeeded.

Lastly, we tell the simulation to run on line 31.


Now that we have a ``.do`` file, lets launch the modelsim GUI:

.. code-block:: console

    $ vsim

and then run our ``.do`` file from the internal console:

.. code-block:: tcl

    ModelSim> do tb.do

Once the simulation completes we can view the results in the waveform view as shown in :ref:`at the top of the page <fig_timing_simulation>`, or process the generated VCD file ``sim.vcd``.
