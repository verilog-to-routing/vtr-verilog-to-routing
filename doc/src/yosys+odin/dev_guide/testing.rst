Testing Yosys+Odin-II
=====================

The ``verify_odin.sh`` script will simulate the microbenchmarks and a larger set of benchmark circuits.
These scripts use simulation results which have been verified against ModelSim.

After you build Yosys+Odin-II, run ``make test_yosys+odin`` to ensure that everything is working correctly on your system.
The ``verify_odin.sh`` also simulates the blif output, as well as simulating the verilog with and without the
architecture file.

Before checking in any changes to Odin-II, please run the script to ensure that it executes correctly.
If there is a failure, use ModelSim to verify that the failure is within Yosys or Odin-II and not a faulty regression test.
If it is a faulty regression test, make an `issue on GitHub <https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/new/choose>`_.
The Odin-II simulator will produce a ``test.do`` file containing clock and input vector information for ModelSim.

When additional circuits are found to agree with ModelSim, they should be added to the regression tests.
When new features are added to Yosys+Odin-II, new microbenchmarks should be developed which test those features for regression.
This process is illustrated in the Developer Guide, in the Regression Tests section.

Using ModelSim to Test Yosys+Odin-II
------------------------------------

ModelSim may be installed as part of the Quartus II Web Edition IDE.
Load the Verilog circuit into a new project in ModelSim.
Compile the circuit, and load the resulting library for simulation.

You may use random vectors via the ``-g`` option, or specify your own input vectors using the ``-t`` option.
When simulation is complete, load the resulting ``test.do`` file into your ModelSim project and execute it.
You may now directly compare the vectors in the ``output_vectors`` file with those produced by ModelSim.

.. note::

	For simulation purposes, you may need to handle the ``GLOBAL_SIM_BASE_CLK`` signal in the ``input_vector`` by either adding this signal as an input signal to the top module or removing it from the ``input_vector`` file.

To add the verified vectors and circuit to an existing test set, move the Verilog file (eg: ``test_circuit.v``) to the test set folder.
Next, move the ``input_vectors`` file to the test set folder, and rename it ``test_circuit_$ELABORATOR_input`` (`$ELABORATOR: odin, yosys`). Finally, move the ``output_vectors`` file to the test set folder and rename it ``test_circuit_$ELABORATOR_output``.
