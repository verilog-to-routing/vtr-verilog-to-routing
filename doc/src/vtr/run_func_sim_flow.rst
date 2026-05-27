.. _run_func_sim_flow:

run_func_sim_flow
-----------------

This script runs the full VTR flow for a single benchmark circuit and architecture file, then verifies the correctness of the resulting post-implementation netlist using `Verilator`_-based functional simulation.

The script is located at::

    $VTR_ROOT/vtr_flow/scripts/run_func_sim_flow.py

.. program:: run_func_sim_flow.py

.. note::
    This script requires Verilator ≥ 5.006.
    On Ubuntu 24.04 and later, Verilator can be installed with ``sudo apt install verilator``.

Basic Usage
~~~~~~~~~~~

At a minimum ``run_func_sim_flow.py`` requires a circuit file, an architecture file, and a testbench::

    run_func_sim_flow.py <circuit_file> <architecture_file> -testbench <testbench_file>

where:

  * ``<circuit_file>`` is the circuit to be processed (Verilog or BLIF)
  * ``<architecture_file>`` is the target :ref:`FPGA architecture <fpga_architecture_description>`
  * ``<testbench_file>`` is a SystemVerilog/Verilog testbench used to verify the post-implementation netlist

All arguments other than :option:`-testbench` and :option:`-verilator_j` are forwarded verbatim to :ref:`run_vtr_flow`.
The flag ``--gen_post_synthesis_netlist on`` is appended automatically so the caller does not need to supply it.

How It Works
~~~~~~~~~~~~

The script performs the following steps:

1. **Run the VTR flow** — invokes :ref:`run_vtr_flow` to synthesize, pack, place, and route the circuit, producing a post-implementation netlist (``<top_module>_post_synthesis.v``).

2. **Compile the simulation** — invokes Verilator to compile the testbench together with the post-implementation netlist and the VTR primitives file (``$VTR_ROOT/vtr_flow/primitives.v``) into a native executable.

3. **Run the simulation** — executes the compiled simulation binary and checks its exit code.

Compilation output is written to ``verilator.out`` and simulation output is written to ``sim.out``, both in the task's run directory.
These files are not printed to the terminal; they are available for inspection when a failure occurs.

.. note::
    VPR names the post-implementation netlist after the **top-level module name** in the source file,
    not after the circuit filename.
    For example, a circuit file ``adder.v`` containing ``module top`` will produce ``top_post_synthesis.v``.

Output
~~~~~~

On success, the script prints a single summary line matching the format used by :ref:`run_vtr_flow`::

    <architecture>/<circuit_name>        OK (took X.XX seconds, overall memory peak XX MiB consumed by vpr run)

On failure the script prints a ``FAILED`` line indicating the stage that failed and the path to the relevant log file::

    <architecture>/<circuit_name>        FAILED: functional simulation (took X.XX seconds, see /path/to/sim.out)

If the VTR flow itself fails, its output is printed unchanged (identical to running :ref:`run_vtr_flow` directly).

Testbench Conventions
~~~~~~~~~~~~~~~~~~~~~

Testbenches must follow these conventions for the script to work correctly:

* The top-level module must be named ``tb``.
* On success, the testbench must call ``$finish`` (causes the simulation to exit with code 0).
* On failure, the testbench must call ``$fatal(1)`` (causes the simulation to exit with a non-zero code).

Example testbench structure:

.. code-block:: systemverilog

    module tb;
        // instantiate the DUT (post-implementation netlist)
        top dut (
            .a(a), .b(b), .sum(sum)
        );

        // drive inputs and check outputs
        initial begin
            // ... apply vectors ...
            if (all_passed)
                $finish;       // success
            else
                $fatal(1);     // failure
        end
    endmodule

Testbench files are stored alongside their corresponding benchmark circuits in::

    $VTR_ROOT/vtr_flow/benchmarks/func_sim/

See the ``README.md`` in that directory for layout conventions and guidance on adding new benchmarks.

Advanced Usage
~~~~~~~~~~~~~~

Additional options can be passed to control the simulation::

    run_func_sim_flow.py <circuit_file> <architecture_file> -testbench <testbench_file> [<options>] [<vtr_flow_options>]

where ``<vtr_flow_options>`` are any arguments not recognized by this script, which are forwarded to :ref:`run_vtr_flow`.

For example::

    run_func_sim_flow.py my_circuit.v my_arch.xml -testbench tb_my_circuit.sv -verilator_j 4 -temp_dir ./my_run

Detailed Command-line Options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. option:: -testbench <file>

    **(Required)** Path to the SystemVerilog or Verilog testbench used to verify the post-implementation netlist.

    The testbench must follow the conventions described in `Testbench Conventions`_.

.. option:: -verilator_j <N>

    Number of parallel threads used by Verilator during compilation of the simulation binary.

    **Default:** number of logical CPU cores on the host machine (``os.cpu_count()``)

.. note::

    All options not recognized by this script are forwarded to :ref:`run_vtr_flow`.
    See :ref:`run_vtr_flow` for the full list of available options, including ``-temp_dir``, ``-starting_stage``, ``-ending_stage``, and VPR options.

Using with Tasks
~~~~~~~~~~~~~~~~

This script is designed to be used as the ``flow_script`` for a :ref:`vtr_tasks` task.
See :ref:`vtr_tasks` for the ``testbench_dir``, ``testbench_file``, and ``flow_script`` configuration keys that enable functional simulation tasks.

.. _Verilator: https://www.veripool.org/verilator/
