# Introduction
The Verilog-to-Routing (VTR) project is a world-wide collaborative effort to provide a open-source framework for conducting FPGA architecture and CAD research and development.
The VTR design flow takes as input a Verilog description of a digital circuit, and a description of the target FPGA architecture.
It then perfoms:
  * Elaboration & Synthesis (ODIN II)
  * Logic Optimization & Technology Mapping (ABC)
  * Packing, Placement, Routing & Timing Analysis (VPR)

to generate FPGA speed and area results.
VTR also includes a set of benchmark designs known to work with the design flow.

# Documentation
VTR's [full documentation](https://docs.verilogtorouting.org) includes tutorials, descriptions of the VTR design flow, and tool options.

Also check out our [additional support resources](SUPPORT.md).

# License
Generally most code is under MIT license, with the exception of ABC which is distributed under its own (permissive) terms.
See the [full license](LICENSE.md) for details.

# Download
For most users of VTR (rather than active developers) you should download the [latest offical VTR release](https://verilogtorouting.org/download), which has been fully regression tested.

# How to Cite
The following paper may be used as a general citation for VTR:

J. Luu, J. Goeders, M. Wainberg, A. Somerville, T. Yu, K. Nasartschuk, M. Nasr, S. Wang, T. Liu, N. Ahmed, K. B. Kent, J. Anderson, J. Rose and V. Betz "VTR 7.0: Next Generation Architecture and CAD System for FPGAs," ACM TRETS, Vol. 7, No. 2, June 2014, pp. 6:1 - 6:30.

Bibtex:
```
@article{vtr2014,
  title={{VTR 7.0: Next Generation Architecture and CAD System for FPGAs}},
  author={Luu, Jason and Goeders, Jeff and Wainberg, Michael and Somerville, Andrew and Yu, Thien and Nasartschuk, Konstantin and Nasr, Miad and Wang, Sen and Liu, Tim and Ahmed, Norrudin and Kent, Kenneth B. and Anderson, Jason and Rose, Jonathan and Betz, Vaughn},
  journal = {ACM Trans. Reconfigurable Technol. Syst.},
  month={June},
  volume={7},
  number={2},
  pages={6:1--6:30},
  year={2014}
}
```

# Building
On unix-like systems run `make` from the root VTR directory.

For more details see the [building instructions](BUILDING.md).

### Docker
We provide a Dockerfile that sets up all the necessary packages for VTR to run.
For more details see [here](dev/DOCKER_DEPLOY.md).

# Mailing Lists
If you have questions, or want to keep up-to-date with VTR, consider joining our mailing lists:

[VTR-Announce](https://groups.google.com/forum/#!forum/vtr-announce): VTR release announcements (low traffic)

[VTR-Users](https://groups.google.com/forum/#!forum/vtr-users): Discussions about using VTR

[VTR-Devel](https://groups.google.com/forum/#!forum/vtr-devel): Discussions about VTR development

[VTR-Commits](https://groups.google.com/forum/#!forum/vtr-commits): VTR revision control commits

# Development
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

## Contributing to VTR
If you'd like to contribute to VTR see our [Contribution Guidelines](CONTRIBUTING.md).

# Contributors
*Please keep this up-to-date*

Professors: Kenneth Kent, Vaughn Betz, Jonathan Rose, Jason Anderson, Peter Jamieson

Research Assistants: Aaron Graham

Graduate Students: Kevin Murray, Jason Luu, Oleg Petelin, Jeffrey Goeders, Chi Wai Yu, Andrew Somerville, Ian Kuon, Alexander Marquardt, Andy Ye, Wei Mark Fang, Tim Liu, Charles Chiasson, Panagiotis (Panos) Patros

Summer Students: Opal Densmore, Ted Campbell, Cong Wang, Peter Milankov, Scott Whitty, Michael Wainberg, Suya Liu, Miad Nasr, Nooruddin Ahmed, Thien Yu, Long Yu Wang, Matthew J.P. Walker, Amer Hesson, Sheng Zhong, Hanqing Zeng, Vidya Sankaranarayanan, Jia Min Wang, Eugene Sha, Jean-Philippe Legault

Companies: Altera Corporation, Texas Instruments
