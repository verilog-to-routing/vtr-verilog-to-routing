SymbiFlow benchmarks
====================

This directory holds all the SymbiFlow benchmarks that are generated in the [SymbiFlow-arch-defs](https://github.com/SymbiFlow/symbiflow-arch-defs) repository.

The circuits come along with the SDC constraints file, if present, and have been produced with yosys.
They are compatible with the symbiflow architectures produced in the same Symbiflow-arch-defs build.

Some of the circuites require also the place constraint files to correctly place some IOs and clock tiles
in the correct location, so not to incur in routability issues.

All the data files can be downloaded with the `make get_symbiflow_benchmarks` target in the root directory.
