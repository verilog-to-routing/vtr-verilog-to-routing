The `Titan <http://www.eecg.utoronto.ca/~kmurray/titan/>` benchmarks and 
architectures are distributed seperately from VTR due to their large size.

To integrate them into VTR run:

    $ make get_titan_benchmarks

from the root of the VTR source tree.

This will download and extract the benchmark netlists to:

    <vtr>/vtr_flow/benchmarks/titan_blif/

and

    <vtr>/vtr_flow/benchmarks/titan_other_blif/

where 'titan_blif' contains the main Titan23 benchmarks, and 'titan_other_blif' contains smaller 
titan-like benchmarks which are useful for testing (but should not be used for architecture and 
CAD evaluation).

The architectures are extracted to:
    <vtr>/vtr_flow/arch/titan/

where <vtr> is the root of the VTR source tree.
