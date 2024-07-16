.. _structure:

Structure
=========

Structure of Parmys Frontend (Yosys + Parmys Plugin)
-------------------------------------------------------------------------

.. code-block:: bash

    └── $VTR_ROOT
        ├── vtr_flow
        │	└── misc
        │		└── parmys
        │			└── synthesis.tcl
        ├── parmys
        │   ├── parmys-plugin
        │   │   ├── core
        │   │   ├── mapping
        │   │   ├── netlist
        │   │   ├── techlibs
        │   │   ├── tests
        │   │   └──utils
        │   ├── test-utils
        │   └── third_party
        └── yosys
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