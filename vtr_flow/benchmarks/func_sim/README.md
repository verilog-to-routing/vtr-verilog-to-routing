# Functional Simulation Benchmarks

This directory contains circuits and testbenches used by the functional
simulation regression suite (`vtr_reg_func_sim`).

Unlike the circuits in `benchmarks/verilog/` and `benchmarks/blif/`, which
are used for quality-of-results (QoR) regression, the circuits here are
small, purpose-built designs whose primary goal is to verify that VPR
correctly implements the intended logic.  Each subdirectory pairs a circuit
with a testbench that exhaustively (or representatively) validates the
post-implementation netlist produced by VPR.

## Layout

```
func_sim/
└── <design_name>/
    ├── <impl_a>.v          # One Verilog implementation (or .blif for pre-synthesized)
    ├── <impl_b>.v          # Another implementation of the same interface
    └── tb_<design_name>.sv # Single Verilator testbench shared by all implementations
```

A design directory groups multiple circuit implementations that share the same
top-level port interface and timing behaviour.  Because all implementations are
functionally equivalent, a single testbench can verify all of them.  Each
implementation is listed as a separate `circuit_list_add` entry in the task
config, and the same `testbench_file` is reused for every run.

Circuit files may be Verilog (`.v`) or pre-synthesized BLIF (`.blif`).  BLIF
circuits enter the VTR flow at the VPR stage (bypassing Yosys and ABC), making
them front-end-independent smoke tests for the simulation pipeline.  Including
BLIF variants alongside Verilog ones confirms that all implementations produce
identical post-P&R results, and it locks in the infrastructure's ability to run
functional simulation on pre-synthesized netlists, which is essential for
future verification of benchmarks that are only distributed
in BLIF format rather than HDL.

## Testbench conventions

All testbenches must follow these conventions so that `run_func_sim_flow.py`
can compile and run them automatically:

1. **Top module name**: the top-level module must be named `tb`.
2. **Success**: call `$finish` when all checks pass (exits with code 0).
3. **Failure**: call `$fatal(1)` when any check fails (exits with non-zero code).
4. **No clock required** for purely combinational designs; drive inputs and
   insert `#1` delays to let combinational outputs settle before sampling.

## Adding a new design

1. Create a subdirectory `func_sim/<design_name>/`.
2. Place one or more circuit source files (`.v` or `.blif`) and a single
   testbench (`tb_<design_name>.sv`) in that subdirectory.
   All circuit files must implement the same top-level port interface.
3. Add a task under
   `vtr_flow/tasks/regression_tests/vtr_reg_func_sim/<design_name>/config/config.txt`
   with one `circuit_list_add` line per implementation and a shared `testbench_file`.
4. Register the task in
   `vtr_flow/tasks/regression_tests/vtr_reg_func_sim/task_list.txt`.

## Adding a new implementation to an existing design

1. Place the new circuit source file (`.v` or `.blif`) in the existing
   `func_sim/<design_name>/` directory.
2. Add a `circuit_list_add=<new_impl>.v` line to the task config.
   No testbench changes are needed.
