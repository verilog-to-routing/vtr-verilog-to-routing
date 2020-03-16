# Verilog to Routing (VTR)
[![Build Status](https://travis-ci.org/verilog-to-routing/vtr-verilog-to-routing.svg?branch=master)](https://travis-ci.org/verilog-to-routing/vtr-verilog-to-routing) [![Documentation Status](https://readthedocs.org/projects/vtr/badge/?version=latest)](http://docs.verilogtorouting.org/en/latest/?badge=latest)

## Introduction
The Verilog-to-Routing (VTR) project is a world-wide collaborative effort to provide a open-source framework for conducting FPGA architecture and CAD research and development.
The VTR design flow takes as input a Verilog description of a digital circuit, and a description of the target FPGA architecture.
It then perfoms:
  * Elaboration & Synthesis (ODIN II)
  * Logic Optimization & Technology Mapping (ABC)
  * Packing, Placement, Routing & Timing Analysis (VPR)

to generate FPGA speed and area results.
VTR also includes a set of benchmark designs known to work with the design flow.

## Documentation
VTR's [full documentation](https://docs.verilogtorouting.org) includes tutorials, descriptions of the VTR design flow, and tool options.

Also check out our [additional support resources](SUPPORT.md).

## License
Generally most code is under MIT license, with the exception of ABC which is distributed under its own (permissive) terms.
See the [full license](LICENSE.md) for details.

## How to Cite
The following paper may be used as a general citation for VTR:

K. E. Murray, O. Petelin, S. Zhong, J. M. Wang, M. ElDafrawy, J.-P. Legault, E. Sha, A. G. Graham, J. Wu, M. J. P. Walker, H. Zeng, P. Patros, J. Luu, K. B. Kent and V. Betz "VTR 8: High Performance CAD and Customizable FPGA Architecture Modelling", ACM TRETS, 2020

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
For most users of VTR (rather than active developers) you should download the [latest offical VTR release](https://verilogtorouting.org/download), which has been fully regression tested.

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

For new developers, please [do the tutorial](dev/tutorial/NewDeveloperTutorial.txt).
You will be directed back here once you ramp up.

VTR development follows a classic centralized repository (svn-like) workflow.
The 'master' branch is supposed to be the most current stable version of the project.
Developers checkout a local copy of the code at the start of development, then do regular updates (e.g. `git pull --rebase`) to keep in sync with the GitHub master.
When a developer has a tested, working change to put back into the trunk, he/she performs a `git push` operation.
Unstable code should remain in the developer's local copy.

We do automated testing of the trunk using BuildBot to verify functionality and Quality of Results (QoR).
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

Graduate Students: Kevin Murray, Jason Luu, Oleg Petelin, Mohamed Eldafrawy, Jeffrey Goeders, Chi Wai Yu, Andrew Somerville, Ian Kuon, Alexander Marquardt, Andy Ye, Wei Mark Fang, Tim Liu, Charles Chiasson, Panagiotis (Panos) Patros

Summer Students: Opal Densmore, Ted Campbell, Cong Wang, Peter Milankov, Scott Whitty, Michael Wainberg, Suya Liu, Miad Nasr, Nooruddin Ahmed, Thien Yu, Long Yu Wang, Matthew J.P. Walker, Amer Hesson, Sheng Zhong, Hanqing Zeng, Vidya Sankaranarayanan, Jia Min Wang, Eugene Sha, Jean-Philippe Legault, Richard Ren, Dingyu Yang

Companies: Intel, Huawei, Lattice, Altera Corporation, Texas Instruments, Google

Funding Agencies: NSERC, Semiconductor Research Corporation
