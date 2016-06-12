The `Titan <http://www.eecg.utoronto.ca/~kmurray/titan/>` benchmarks and 
architectures are distributed seperately from VTR due to their large size.

To integrate them into VTR run:

    $ make get_titan_benchmarks

from the root of the VTR source tree.

This will download and extract the benchmark netlists to:
    <vtr>/vtr_flow/benchmarks/titan_blif/

and the architectures to:
    <vtr>/vtr_flow/arch/titan/

where <vtr> is the root of the VTR source tree.
