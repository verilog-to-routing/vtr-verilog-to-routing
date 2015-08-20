Tatum: A Fast, Flexible Static Timing Analysis (STA) Engine for Digital Circuits
================================================================================
##Overview
Tatum is primarily a library (`libtatum`) which provides a fast and flexible Static Timing Analysis (STA) engine for digital circuits.

Tatum is a block-based timing analyzer suitable for integration with Computer-Aided Design (CAD) tools used to optimize and implement digital circuits.
Tatum supports both setup (max) and hold (min) analysis, clock skew and multiple clocks.

Tatum operates on an abstract *timing graph* constructed by the host application, can be configured to use an application defined delay calculator.

Tatum is optimized for high performance, as required by optimizing CAD tools.
In particular:
  * Tatum performs only a single set of graph traversals to calculate timing information for all clocks.
  * Tatum's data structures are cache optimized
  * Tatum supports parallel analysis using multiple CPU cores

##Why was Tatum created?
I had need for a high performance, flexible STA engine for my PhD research into FPGA architecture and CAD tools.
I could find no suitable open source STA engines, and wrote my own.

##Origin of the Name
A *Tatum* is a unit of time used in the computational analysis of music.
It is "roughly equivalent to the time division that most highly coincides with note onsets" \[[1]\].
Tatums are named after the famous Jazz pianist [Art Tatum](https://en.wikipedia.org/wiki/Art_Tatum).

[1]: http://web.media.mit.edu/~tristan/phd/dissertation/chapter3.html#x1-390003.4.3
