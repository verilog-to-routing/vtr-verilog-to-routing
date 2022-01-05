.. _structure:

Structure
=========

Structure of Yosys Synthesis Files and the Yosys External Library Library 
-------------------------------------------------------------------------

.. code-block:: bash

    └── $VTR_ROOT
    	├── vtr_flow
    	│	└── misc
    	│		└── yosyslib
    	│			├── adder.v
    	│			├── dpram_rename.v
    	│			├── dual_port_ram.v
    	│			├── multiply.v
    	│			├── single_port_ram.v
    	│			├── spram_rename.v
    	│			├── synthesis.ys
    	│			└── yosys_models.v
    	└── libs
    	    └── EXTERNAL
    	        └── libyosys
                    ├── backends
                    ├── examples
                    ├── frontends
                    ├── guidelines
                    ├── kernel
                    ├── libs
                    ├── manual
                    ├── misc
                    ├── passes
                    ├── techlibs
                    └── tests