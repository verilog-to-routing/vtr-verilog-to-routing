The `Titan <http://www.eecg.utoronto.ca/~kmurray/titan/>` benchmarks are distributed seperately from VTR due to their large size.

The Titan repo is located under /home/kmurray/trees/titan on the U of T EECG network. Memebers of Vaughn Betz's research lab have read/write privileges. 

This repo is where the Titan flow is developed and where any changes to it should be made.

In addition to the titan benchmarks, this repo contains scripts that are used ingeneration of the architecture description for Stratix IV.

More specifically, they contain scripts that generate memory blocks & complex switch blocks.

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
