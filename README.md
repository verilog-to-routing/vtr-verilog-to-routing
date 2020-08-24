# Verilog to Routing (VTR)
[![Build Status](https://travis-ci.com/verilog-to-routing/vtr-verilog-to-routing.svg?branch=master)](https://travis-ci.com/verilog-to-routing/vtr-verilog-to-routing) [![Documentation Status](https://readthedocs.org/projects/vtr/badge/?version=latest)](http://docs.verilogtorouting.org/en/latest/)

## Introduction
The Verilog-to-Routing (VTR) project is a world-wide collaborative effort to provide a open-source framework for conducting FPGA architecture and CAD research and development.
The VTR design flow takes as input a Verilog description of a digital circuit, and a description of the target FPGA architecture.
It then performs:
  * Elaboration & Synthesis (ODIN II)
  * Logic Optimization & Technology Mapping (ABC)
  * Packing, Placement, Routing & Timing Analysis (VPR)

to generate FPGA speed and area results.
VTR includes a set of benchmark designs known to work with the design flow.

VTR can also produce [FASM](https://symbiflow.readthedocs.io/en/latest/fasm/docs/specification.html) to program some commercial FPGAs (via [Symbiflow](https://symbiflow.github.io/))

| Placement (carry-chains highlighted) | Critical Path |
| ------------------------------------ | ------------- |
| <img src="https://verilogtorouting.org/img/des90_placement_macros.gif" width="350"/> | <img src="https://verilogtorouting.org/img/des90_cpd.gif" width="350"/> | 

| Logical Connections | Routing Utilziation |
| ------------------- | ------------------- |
| <img src="https://verilogtorouting.org/img/des90_nets.gif" width="350"/> | <img src="https://verilogtorouting.org/img/des90_routing_util.gif" width="350"/> |


## Documentation
VTR's [full documentation](https://docs.verilogtorouting.org) includes tutorials, descriptions of the VTR design flow, and tool options.

Also check out our [additional support resources](SUPPORT.md).

## License
Generally most code is under MIT license, with the exception of ABC which is distributed under its own (permissive) terms.
See the [full license](LICENSE.md) for details.

## How to Cite
The following paper may be used as a general citation for VTR:

K. E. Murray, O. Petelin, S. Zhong, J. M. Wang, M. ElDafrawy, J.-P. Legault, E. Sha, A. G. Graham, J. Wu, M. J. P. Walker, H. Zeng, P. Patros, J. Luu, K. B. Kent and V. Betz "VTR 8: High Performance CAD and Customizable FPGA Architecture Modelling", ACM TRETS, 2020.

Bibtex:
```
@article{vtr8,
  title={VTR 8: High Performance CAD and Customizable FPGA Architecture Modelling},
  author={Murray, Kevin E. and Petelin, Oleg and Zhong, Sheng and Wang, Jai Min and ElDafrawy, Mohamed and Legault, Jean-Philippe and Sha, Eugene and Graham, Aaron G. and Wu, Jean and Walker, Matthew J. P. and Zeng, Hanqing and Patros, Panagiotis and Luu, Jason and Kent, Kenneth B. and Betz, Vaughn},
  journal={ACM Trans. Reconfigurable Technol. Syst.},
  year={2020}
}
```

## Download
For most users of VTR (rather than active developers) you should download the [latest official VTR release](https://verilogtorouting.org/download), which has been fully regression tested.

## Building
On unix-like systems run `make` from the root VTR directory.

For more details see the [building instructions](BUILDING.md).

#### Docker
We provide a Dockerfile that sets up all the necessary packages for VTR to run.
For more details see [here](dev/DOCKER_DEPLOY.md).

## Mailing Lists
If you have questions, or want to keep up-to-date with VTR, consider joining our mailing lists:

[VTR-Announce](https://groups.google.com/forum/#!forum/vtr-announce): VTR release announcements (low traffic)

[VTR-Users](https://groups.google.com/forum/#!forum/vtr-users): Discussions about using VTR

[VTR-Devel](https://groups.google.com/forum/#!forum/vtr-devel): Discussions about VTR development

[VTR-Commits](https://groups.google.com/forum/#!forum/vtr-commits): VTR revision control commits

## Development
This is the development trunk for the Verilog-to-Routing project.
Unlike the nicely packaged releases that we create, you are working with code in a constant state of flux.
You should expect that the tools are not always stable and that more work is needed to get the flow to run.

For new developers, please follow the [quickstart guide](https://docs.verilogtorouting.org/en/latest/quickstart/).

We follow a feature branch flow, where you create a new branch for new code, test it, measure its Quality of Results, and eventually produce a pull request for review by other developers. Pull requests that meet all the quality and review criteria are then merged into the master branch by a developer with the authority to do so.

In addition to measuring QoR and functionality automatically on pull requests, we do periodic automated testing of the master using BuildBot, and the results can be viewed below to track QoR and stability.
* [Trunk Status](http://builds.verilogtorouting.org:8080/waterfall)
* [QoR Tracking](http://builds.verilogtorouting.org:8080/)

*IMPORTANT*: A broken build must be fixed at top priority. You break the build if your commit breaks any of the automated regression tests.

For additional information see the [developer README](README.developers.md).

### Contributing to VTR
If you'd like to contribute to VTR see our [Contribution Guidelines](CONTRIBUTING.md).

## Contributors
*Please keep this up-to-date*

Professors: Kenneth Kent, Vaughn Betz, Jonathan Rose, Jason Anderson, Peter Jamieson

Research Assistants: Aaron Graham


Graduate Students: Kevin Murray, Jason Luu, Oleg Petelin, Xifian Tang, Mohamed Elgammal, Mohamed Eldafrawy, Jeffrey Goeders, Chi Wai Yu, Andrew Somerville, Ian Kuon, Alexander Marquardt, Andy Ye, Wei Mark Fang, Tim Liu, Charles Chiasson, Panagiotis (Panos) Patros, Jean-Philippe Legault, Aaron Graham, Nasrin Eshraghi Ivari, Maria Patrou, Scott Young, Sarah Khalid, Seyed Alireza Damghani


Summer Students: Opal Densmore, Ted Campbell, Cong Wang, Peter Milankov, Scott Whitty, Michael Wainberg, Suya Liu, Miad Nasr, Nooruddin Ahmed, Thien Yu, Long Yu Wang, Matthew J.P. Walker, Amer Hesson, Sheng Zhong, Hanqing Zeng, Vidya Sankaranarayanan, Jia Min Wang, Eugene Sha, Jean-Philippe Legault, Richard Ren, Dingyu Yang, Alexandrea Demmings, Hillary Soontiens, Julie Brown, Bill Hu, David Baines, Mahshad Farahani, Helen Dai, Daniel Zhai

Companies: Intel, Huawei, Lattice, Altera Corporation, Texas Instruments, Google, Antmicro

Funding Agencies: NSERC, Semiconductor Research Corporation


