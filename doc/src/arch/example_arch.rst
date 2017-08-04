Example Architecture Specification
==================================

The listing below is for an FPGA with I/O pads, soft logic blocks (called CLB), configurable memory hard blocks, and fracturable multiplier hard blocks.

Notice that for the CLB, all the inputs are logically equivalent (line 157), and all the outputs are logically equivalent (line 158).
This is usually true for cluster-based logic blocks, as the local routing within the block usually provides full (or near full) connectivity.

However, for other logic blocks, the inputs and all the outputs are not logically equivalent.
For example, consider the memory block (lines 311-316).
Swapping inputs going into the data input port changes the logic of the block because the data output order no longer matches the data input.

.. literalinclude:: example_arch.xml
    :language: xml
    :linenos:
    :emphasize-lines: 157-158, 217-218, 311-316

