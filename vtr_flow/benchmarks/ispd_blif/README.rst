The `ISPD16 <http://www.ispd.cc/contests/16>` and `ISPD17 <http://www.ispd.cc/contests/17>` benchmarks 
(converted to be compatible with VPR) are distributed seperately from VTR due to their large size.

To integrate them into VTR run:

    $ make get_ispd_benchmarks

from the root of the VTR source tree.

This will download and extract the benchmark netlists to:

    <vtr>/vtr_flow/benchmarks/ispd_blif/

The relevant architecture to use is:
    <vtr>/vtr_flow/arch/ispd/ultrascale_ispd.xml

where <vtr> is the root of the VTR source tree.

However note that the ISPD contest architectures have no timing model (so no timing-driven optimization) and focused on placement only (no routing architecture).
