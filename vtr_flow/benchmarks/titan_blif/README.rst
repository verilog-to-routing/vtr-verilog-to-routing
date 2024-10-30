The `Titan <http://www.eecg.utoronto.ca/~vaughn/titan/>` benchmarks are distributed separately from VTR due to their large size.

The Titan repo is located at /home/vaughn/titan/titan.git on the U of T EECG network. Members of Vaughn Betz's research lab have read/write privileges.

This repo is where the Titan flow is developed and where any changes to it should be made.

In addition to the titan benchmarks, this repo contains scripts that are used in generation of the architecture description for Stratix IV.

More specifically, they contain scripts that generate memory blocks & complex switch blocks. 


To integrate them into VTR run:

    $ make get_titan_benchmarks

from the root of the VTR source tree.

This will download and extract the benchmark netlists to:

    <vtr>/vtr_flow/benchmarks/titan_blif/


In the directory 'titan_blif', you will find three primary subdirectories that 
correspond to three sets of main benchmarks: 'titan23', 'titan_new', and 'other_benchmarks'. 
Each of these subdirectories contains two further directories, namely 'stratixiv' and 'stratix10'. 
These nested directories house the BLIF files specific to each device. 
The 'titan_other_blif' contains smaller titan-like benchmarks which are useful for 
testing (but should not be used for architecture and CAD evaluation).
