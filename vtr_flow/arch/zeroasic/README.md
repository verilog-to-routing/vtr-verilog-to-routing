# Zero ASIC Architectures

These are the VTR captures of the Zero ASIC architectures.

The orginal Zero ASIC architectures can be found in logiklib here:
https://github.com/siliconcompiler/logiklib

These architectures have been slightly modified to work with VTR's CAD flow
(i.e. synthesis) and VTR's benchmark suites.

The RR graphs of these architectures are required in order to route circuits
on them. These RR graphs can be very large, so they are stored in zip format.
To unzip them, run the following command in the root VTR directory:

```sh
make get_zeroasic_rr_graphs
```
