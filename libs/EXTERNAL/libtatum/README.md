# Tatum: A Fast, Flexible Static Timing Analysis (STA) Engine for Digital Circuits

[![Build Status](https://travis-ci.org/kmurray/tatum.svg?branch=master)](https://travis-ci.org/kmurray/tatum)

## Overview
Tatum is a block-based Static Timing Analysis (STA) engine suitable for integration with Computer-Aided Design (CAD) tools, which analyze, implement and optimize digital circuits.
Tatum supports both setup (max-delay) and hold (min-delay) analysis, clock skew, multiple clocks and a variety of timing exceptions.

Tatum is provided as a library (`libtatum`) which can be easily integrated into the host application.
Tatum operates on an abstract *timing graph* constructed by the host application, and can be configured to use an application defined delay calculator.

Tatum is optimized for high performance, as required by optimizing CAD tools.
In particular:
  * Tatum performs only a *single* set of graph traversals to calculate timing information for all clocks and analyses (setup and hold).
  * Tatum's data structures are cache optimized
  * Tatum supports parallel analysis using multiple CPU cores

## How to Cite
If your work uses Tatum please cite the following as a general citation:

K. E. Murray and V. Betz, "Tatum: Parallel Timing Analysis for Faster Design Cycles and Improved Optimization", *IEEE International Conference on Field-Programmable Technology (FPT)*, 2018

**Bibtex:**
```
@inproceedings{c:tatum,
    author = {Murray, Kevin E. and Betz, Vaughn},
    title = {Tatum: Parallel Timing Analysis for Faster Design Cycles and Improved Optimization},
    booktitle = {IEEE International Conference on Field-Programmable Technology (FPT)},
    year = {2018}
}
```

## Documentation
Comming soon.

## Download
Comming soon.

## Projects using Tatum

Tatum is designed to be re-usable in a variety of appliations.

Some of the known uses are:
  * The [Verilog to Routing (VTR)](https://verilogtorouting.org) project for Field-Programmable Gate Array (FPGA) Architecture and CAD research. Tatum is used as the STA engine in the VPR placement and routing tool.
  * The [CGRA-ME](http://cgra-me.ece.utoronto.ca/) framework for Coarse-Grained Reconfigurable Array (CGRA) Architecture research.

*If your project is using Tatum please let us know!*

## History

### Why was Tatum created?
I had need for a high performance, flexible STA engine for my research into FPGA architecture and CAD tools.
I could find no suitable open source STA engines, and wrote my own.

### Name Origin
A *tatum* is a unit of time used in the computational analysis of music \[[1]\], named after Jazz pianist [Art Tatum](https://en.wikipedia.org/wiki/Art_Tatum).

[1]: http://web.media.mit.edu/~tristan/phd/dissertation/chapter3.html#x1-390003.4.3
