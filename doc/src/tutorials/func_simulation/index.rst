.. _func_simulation_tutorial:

Post-Implementation Functional Simulation
------------------------------------------

This tutorial describes how to verify the functional correctness of a circuit
implemented by :ref:`VPR` using `Verilator <https://www.veripool.org/verilator/>`__,
an open-source hardware simulator.

Functional simulation is useful for:

* Verifying that the VTR flow has correctly implemented the circuit's intended logic
* Quickly checking correctness before running a full :ref:`timing simulation <timing_simulation_tutorial>`
* Debugging unexpected circuit behaviour after implementation

Unlike :ref:`timing simulation <timing_simulation_tutorial>`, functional simulation
does not require a Standard Delay Format (SDF) file; only the
post-implementation Verilog netlist and VTR's primitive definitions are needed.

.. note::
    This tutorial requires Verilator 5.006 or later.
    Verilator can be installed from your distribution's package manager or
    built from source: https://github.com/verilator/verilator

    On Ubuntu, install it with:

    .. code-block:: console

        $ sudo apt install verilator


The Design
~~~~~~~~~~

The circuit for this tutorial is a **4-bit ripple-carry adder** with the
following ports:

* **Inputs**: ``cin`` (carry-in), ``a0``-``a3`` and ``b0``-``b3``
  (4-bit operands, ``a0`` / ``b0`` are the least-significant bits)
* **Outputs**: ``s0``-``s3`` (sum bits, ``s0`` least-significant) and ``cout``
  (carry-out)

When synthesis compiles this verilog file to a blif file, buses get turned
into individual bits rather than ports. Using individual single-bit ports
(rather than bus ports such as ``input [3:0] a``) keeps the post-implementation
netlist port names simple and easy to reference in the testbench.

Create a working directory and save the design as ``top.v``:

.. code-block:: console

    $ mkdir func_sim_tut
    $ cd func_sim_tut

.. code-block:: verilog
    :linenos:
    :caption: 4-bit ripple-carry adder ``top.v``.

    module top (
        input  cin,
        input  a0, a1, a2, a3,
        input  b0, b1, b2, b3,
        output s0, s1, s2, s3,
        output cout
    );
        wire c0, c1, c2;

        // Bit 0
        assign s0 = a0 ^ b0 ^ cin;
        assign c0 = (a0 & b0) | ((a0 ^ b0) & cin);

        // Bit 1
        assign s1 = a1 ^ b1 ^ c0;
        assign c1 = (a1 & b1) | ((a1 ^ b1) & c0);

        // Bit 2
        assign s2 = a2 ^ b2 ^ c1;
        assign c2 = (a2 & b2) | ((a2 ^ b2) & c1);

        // Bit 3
        assign s3 = a3 ^ b3 ^ c2;
        assign cout = (a3 & b3) | ((a3 ^ b3) & c2);
    endmodule


Generating the Post-Implementation Netlist
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Copy the VTR flagship architecture into the working directory:

.. code-block:: console

    $ cp $VTR_ROOT/vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml .

.. note:: Replace :term:`$VTR_ROOT` with the root directory of the VTR source tree.

Run the full VTR flow using ``run_vtr_flow.py``.
The :option:`vpr --gen_post_synthesis_netlist` option instructs VPR to write
the post-implementation Verilog netlist alongside the usual place-and-route
output files:

.. code-block:: console

    $ source $VTR_ROOT/.venv/bin/activate
    $ python3 $VTR_ROOT/vtr_flow/scripts/run_vtr_flow.py \
    $     top.v \
    $     k6_frac_N10_frac_chain_mem32K_40nm.xml \
    $     --gen_post_synthesis_netlist on

After the flow completes you should see the post-implementation netlist:

.. code-block:: console

    $ ls temp/top_post_synthesis.v temp/top_post_synthesis.sdf
    top_post_synthesis.sdf  top_post_synthesis.v

.. note::
    The SDF file (``top_post_synthesis.sdf``) contains back-annotated timing
    delays and is used for :ref:`timing simulation <timing_simulation_tutorial>`.
    Functional simulation does not require it.


Inspecting the Post-Implementation Netlist
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The generated netlist expresses the circuit using FPGA primitives defined in
``$VTR_ROOT/vtr_flow/primitives.v``.  Below is a representative snippet:

.. code-block:: verilog
    :caption: Snippet of ``top_post_synthesis.v``.
    :emphasize-lines: 1,20-23

    module top (
        input \cin ,
        input \a0 ,
        input \a1 ,
        input \a2 ,
        input \a3 ,
        input \b0 ,
        input \b1 ,
        input \b2 ,
        input \b3 ,
        output \s0 ,
        output \s1 ,
        output \s2 ,
        output \s3 ,
        output \cout
    );

        // ... wire declarations and interconnect ...

        LUT_K #(
            .K(5),
            .LUT_MASK(32'b00000000010000010000000000010100)
        ) \lut_s0  (
            .in({ \lut_s0_input_0_4 , 1'bX, \lut_s0_input_0_2 ,
                  \lut_s0_input_0_1 , 1'bX }),
            .out(\lut_s0_output_0_0 )
        );

        // ... more LUT_K cells ...

    endmodule

The module is named after the top-level module of the design being packed, placed, and routed by VTR (``top`` in this tutorial).
Port names beginning with ``\`` are standard Verilog *escaped identifiers*;
``\cin`` (with a trailing space) is the same identifier as plain ``cin``, so they connect to
testbench signals named without the backslash.

The primitives used here, ``LUT_K`` (look-up table) and
``fpga_interconnect`` (routing wire), are defined in ``primitives.v``.
If the architecture's carry chain is used, ``adder`` primitives from
``primitives.v`` may also appear.


Creating a Testbench
~~~~~~~~~~~~~~~~~~~~

The testbench ``tb.sv`` drives every possible combination of the 9 input
bits (2\ :sup:`9` = 512 vectors), computes the expected sum using ordinary
arithmetic, and flags any mismatch:

.. code-block:: systemverilog
    :linenos:
    :caption: Testbench ``tb.sv``.
    :emphasize-lines: 16-22,36-37,39-43,50-56

    `timescale 1ns/1ps
    module tb;

        // DUT I/O
        logic cin;
        logic a0, a1, a2, a3;
        logic b0, b1, b2, b3;
        logic s0, s1, s2, s3, cout;

        logic [4:0] expected, actual;
        int errors;

        // Instantiate the post-implementation netlist.
        // The escaped port names (\cin, \a0, ...) are identical to the
        // unescaped names used below, so the named connections work directly.
        top dut (
            .cin (cin),
            .a0  (a0), .a1 (a1), .a2 (a2), .a3 (a3),
            .b0  (b0), .b1 (b1), .b2 (b2), .b3 (b3),
            .s0  (s0), .s1 (s1), .s2 (s2), .s3 (s3),
            .cout(cout)
        );

        initial begin
            errors = 0;

            // Sweep all 2^9 = 512 input combinations
            for (int c = 0; c < 2; c++) begin
                cin = c[0];
                for (int a = 0; a < 16; a++) begin
                    {a3, a2, a1, a0} = 4'(a);
                    for (int b = 0; b < 16; b++) begin
                        {b3, b2, b1, b0} = 4'(b);
                        #1; // allow combinational outputs to settle

                        expected = 5'(a + b + c);
                        actual   = {cout, s3, s2, s1, s0};

                        if (actual !== expected) begin
                            $display("FAIL: a=%0d b=%0d cin=%0d  expected=%0d  got=%0d",
                                     a, b, c, expected, actual);
                            errors++;
                        end
                    end
                end
            end

            if (errors == 0) begin
                $display("All 512 tests PASSED.");
                $finish;          // exit 0 — success
            end else begin
                $display("%0d test(s) FAILED.", errors);
                $fatal(1);        // exit 1 — failure
            end
        end

    endmodule

Lines 16-22 instantiate the DUT (Device Under Test), the post-implementation
netlist module ``top``, using named port connections.
Line 36 computes the expected 5-bit result using integer arithmetic, and
line 37 assembles the actual 5-bit result from the individual DUT output bits.
Lines 39-43 use ``$display`` (not ``$error``) to report each mismatch.
``$display`` is non-fatal, so every one of the 512 vectors is checked and
all failures are reported before the simulation exits.
Lines 50-56 select the final exit path: ``$finish`` exits with code 0
(success) and ``$fatal(1)`` exits with code 1 (failure), making the
simulation suitable for use as a CI test.


Simulating with Verilator
~~~~~~~~~~~~~~~~~~~~~~~~~

Compile the testbench, post-implementation netlist, and VTR primitives together.
Verilator's ``--binary`` flag produces a standalone simulation executable in a
single step:

.. code-block:: console

    $ verilator --binary -sv \
    $     tb.sv \
    $     temp/top_post_synthesis.v \
    $     $VTR_ROOT/vtr_flow/primitives.v \
    $     --top-module tb \
    $     --Mdir sim_build \
    $     -j $(nproc)

This compiles ``tb.sv``, the post-implementation netlist, and the VTR
primitives into a standalone simulation binary under ``sim_build/``.
The ``-j $(nproc)`` flag parallelises the build across all available CPU cores.
This generates the simulation binary ``sim_build/Vtb``.
Run it to execute all 512 test vectors:

.. code-block:: console

    $ ./sim_build/Vtb
    All 512 tests PASSED.

.. note::
    ``primitives.v`` uses Verilog ``specify`` blocks to model timing.
    Verilator ignores these for functional simulation, so no timing information
    is back-annotated, which is the desired behaviour here.

If any test vector fails, the simulation prints each failure, reports the
total count, and exits with a non-zero return code:

.. code-block:: console

    FAIL: a=3 b=5 cin=0  expected=8  got=7
    1 test(s) FAILED.
    %Fatal: tb.sv:53: $fatal called

Such an error would indicate a bug in VPR's implementation of the circuit
(assuming the input Verilog design and the testbench are coded correctly/match),
which can then be investigated by examining the post-implementation netlist
or running the full :ref:`timing simulation <timing_simulation_tutorial>`
with waveform capture.
