# TESTING ODIN II

The verify_odin.sh script will simulate the microbenchmarks and a larger set of benchmark circuits.
These scripts use simulation results which have been verified against ModelSim.

After you build Odin II, run 'make test' to ensure that everything is working correctly on your system. 
Unlike the verify\_regression\_tests.sh script, verify\_odin.sh also simulates the
blif output, as well as simulating the verilog with and without the
architecture file.

Before checking in any changes to Odin II, please run both of these scripts to ensure that both of these scripts execute correctly.
If there is a failure, use ModelSim to verify that the failure is within Odin II and not a faulty regression test.
If it is a faulty regression test, make an [issue on GitHub](./reporting_bugs.md).
The Odin II simulator will produce a test.do file containing clock and input vector information for ModelSim.

When additional circuits are found to agree with ModelSim, they should be added to the regression tests.
When new features are added to Odin II, new microbenchmarks should be developed which test those features for regression.
This process is illustrated in the Developper Guide, in the Regression Tests section.

## USING MODELSIM TO TEST ODIN II

ModelSim may be installed as part of the Quartus II Web Edition IDE.
Load the Verilog circuit into a new project in ModelSim.
Compile the circuit, and load the resulting library for simulation.

You may use random vectors via the -g option, or specify your own input vectors using the -t option.
When simulation is complete, load the resulting test.do file into your ModelSim project and execute it.
You may now directly compare the vectors in the output\_vectors file with those produced by ModelSim.

To add the verified vectors and circuit to an existing test set, move the verilog file (eg: test\_circuit.v) to the test set folder.
Next, move the input\_vectors file to the test set folder, and rename it test\_circuit\_input. Finally, move the output\_vectors file to the test set folder and rename it test\_circuit\_output.
