# VTR Change Log
<!-- 
This file documents user-facing changes between releases of the VTR
project. The goal is to concicely communicate to end users what is new
or has changed in a particular release. It should *NOT* just be a dump
of the commit log, as that is far too detailed. Most code re-factoring
does not warrant a change log entry unless it has a significant impact
on the end users (e.g. substantial performance improvements).

Each release's change log should include headings (where releveant) with
bullet points listing what was: 
  - added           (new feature)
  - changed         (change to existing feature behaviour)
  - fixed           (bug fix)
  - deprecated      (features planned for future removal)
  - removed         (previous features which have been removed)

Changes which have landed in the master/trunk but not been released
should be included in the 'Unreleased' section and moved to the releveant
releases' section when released.

In the case of release candidates (e.g. v8.0.0-rc1) the current
set of unreleased changes should be moved under that heading. Any
subsequent fixes to the release candidate would be placed under
'Unreleased', eventually moving into the next release candidate's
heading (e.g. v8.0.0-rc2) when created. Note this means the change log for
subsequent release candidates (e.g. rc2) would only contain new changes
not included in previous release candidates (e.g. rc1).  When the final
(non-release candidate) release is made (e.g. v8.0.0) the change log
should contain all the relevant changes compared to the last non-release
candidate release (e.g. v7.0.0). That is, it should be the concatenation
of the unreleased and any previous release candidates change logs.
-->

_Note that changes from release candidates (e.g. v8.0.0-rc1, v8.0.0-rc2) are included/repeated in the final release (e.g. v8.0.0) change log._

<!--
## Unreleased
_The following are changes which have been implemented in the VTR master branch but have not yet been included in an official release._

### Added

### Changed

### Fixed

### Deprecated

### Removed
-->

## v8.0.0 - 2020-03-24

### Added
 * Support for arbitrary FPGA device grids/floorplans
 * Support for clustered blocks with width > 1
 * Customizable connection-block and switch-blocks patterns (controlled from FPGA architecture file)
 * Fan-out dependent routing mux delays
 * VPR can generate/load a routing architecture (routing resource graph) in XML format
 * VPR can load routing from a `.route` file
 * VPR can performing analysis (STA/Power/Area) independently from optimization (via `vpr --analysis`)
 * VPR supports netlist primitives with multiple clocks
 * VPR can perform hold-time (minimum delay) timing analysis
 * Minimum delays can be annotated in the FPGA architecture file
 * Flow supports formal verification of circuit implementation against input netlist
 * Support for generating FASM to drive bitstream generators
 * Routing predictor which predicts and aborts impossible routings early (saves significant run-time during minimum channel width search)
 * Support for minimum routable channel width 'hints' (reduces minimum channel width search run-time if accurate)
 * Improved VPR debugging/verbosity controls
 * VPR can perform basic netlist cleaning (e.g. sweeping dangling logic)
 * VPR graphics visualizations:
   * Critical path during placement/routing
   * Cluster pin utilization heatmap
   * Routing utilization heatmap
   * Routing resource cost heatmaps
   * Placement macros
 * VPR can route constant nets
 * VPR can route clock nets
 * VPR can load netlists in extended BLIF (eBLIF) format
 * Support for generating post-placement timing reports
 * Improved router 'map' lookahead which adapts to routing architecture structure
 * Script to upgrade legacy architecture files (`vtr_flow/scripts/upgrade_arch.py`)
 * Support for Fc overrides which depend on both pin and target wire segment type
 * Support for non-configurable switches (shorts, inline-buffers) used to model structures like clock-trees and non-linear wires (e.g. 'L' or 'T' shapes)
 * Various other features since VTR 7

### Changed
 * VPR will exit with code 1 on errors (something went wrong), and code 2 when unable to implement a circuit (e.g. unroutable)
 * VPR now gives more complete help about command-line options (`vpr -h`)
 * Improved a wide variety of error messages
 * Improved STA timing reports (more details, clearer format)
 * VPR now uses Tatum as its STA engine
 * VPR now detects missmatched architecture (.xml) and implementation (.net/.place/.route) files more robustly
 * Improved router run-time and quality through incremental re-routing and improved handling of high-fanout nets
 * The timing edges within each netlist primitive must now be specified in the <models> section of the architecture file
 * All interconnect tags must have unique names in the architecture file
 * Connection block input pin switch must now be specified in <switchlist> section of the architecture file
 * Renamed switch types buffered/pass_trans to more descriptive tristate/pass_gate in architecture file
 * Require longline segment types to have no switchblock/connectionblock specification
 * Improve naming (true/false -> none/full/instance) and give more control over block pin equivalnce specifications
 * VPR will produce a .route file even if the routing is illegal (aids debugging), however analysis results will not be produced unless `vpr --analsysis` is specified
 * VPR long arguments are now always prefixed by two dashes (e.g. `--route`) while short single-letter arguments are prefixed by a single dash (e.g. `-h`)
 * Improved logic optimization through using a recent 2018 version of ABC and new synthesis script
 * Significantly improved implementation quality (~14% smaller minimum routable channel widths, 32-42% reduced wirelength, 7-10% lower critical path delay)
 * Significantly reduced run-time (~5.5-6.3x faster) and memory usage (~3.3-5x lower)
 * Support for non-contiguous track numbers in externally loaded RR graphs
 * Improved placer quality (reduced cost round-off)
 * Various other changes since VTR 7

### Fixed
 * FPGA Architecture file tags can be in arbitary orders
 * SDC command arguments can be in arbitary orders
 * Numerous other fixes since VTR 7

### Removed
 * Classic VPR timing analyzer
 * IO channel distribution section of architecture file

### Deprecated
 * VPR's breadth-first router (use the timing-driven router, which provides supperiour QoR and Run-time)

## v8.0.0-rc2 - 2019-08-01

### Changed
 * Support for non-contiguous track numbers in externally loaded RR graphs
 * Improved placer quality (reduced cost round-off)

## v8.0.0-rc1 - 2019-06-13

### Added
 * Support for arbitrary FPGA device grids/floorplans
 * Support for clustered blocks with width > 1
 * Customizable connection-block and switch-blocks patterns (controlled from FPGA architecture file)
 * Fan-out dependent routing mux delays
 * VPR can generate/load a routing architecture (routing resource graph) in XML format
 * VPR can load routing from a `.route` file
 * VPR can performing analysis (STA/Power/Area) independently from optimization (via `vpr --analysis`)
 * VPR supports netlist primitives with multiple clocks
 * VPR can perform hold-time (minimum delay) timing analysis
 * Minimum delays can be annotated in the FPGA architecture file
 * Flow supports formal verification of circuit implementation against input netlist
 * Support for generating FASM to drive bitstream generators
 * Routing predictor which predicts and aborts impossible routings early (saves significant run-time during minimum channel width search)
 * Support for minimum routable channel width 'hints' (reduces minimum channel width search run-time if accurate)
 * Improved VPR debugging/verbosity controls
 * VPR can perform basic netlist cleaning (e.g. sweeping dangling logic)
 * VPR graphics visualizations:
   * Critical path during placement/routing
   * Cluster pin utilization heatmap
   * Routing utilization heatmap
   * Routing resource cost heatmaps
   * Placement macros
 * VPR can route constant nets
 * VPR can route clock nets
 * VPR can load netlists in extended BLIF (eBLIF) format
 * Support for generating post-placement timing reports
 * Improved router 'map' lookahead which adapts to routing architecture structure
 * Script to upgrade legacy architecture files (`vtr_flow/scripts/upgrade_arch.py`)
 * Support for Fc overrides which depend on both pin and target wire segment type
 * Support for non-configurable switches (shorts, inline-buffers) used to model structures like clock-trees and non-linear wires (e.g. 'L' or 'T' shapes)
 * Various other features since VTR 7

### Changed
 * VPR will exit with code 1 on errors (something went wrong), and code 2 when unable to implement a circuit (e.g. unroutable)
 * VPR now gives more complete help about command-line options (`vpr -h`)
 * Improved a wide variety of error messages
 * Improved STA timing reports (more details, clearer format)
 * VPR now uses Tatum as its STA engine
 * VPR now detects missmatched architecture (.xml) and implementation (.net/.place/.route) files more robustly
 * Improved router run-time and quality through incremental re-routing and improved handling of high-fanout nets
 * The timing edges within each netlist primitive must now be specified in the <models> section of the architecture file
 * All interconnect tags must have unique names in the architecture file
 * Connection block input pin switch must now be specified in <switchlist> section of the architecture file
 * Renamed switch types buffered/pass_trans to more descriptive tristate/pass_gate in architecture file
 * Require longline segment types to have no switchblock/connectionblock specification
 * Improve naming (true/false -> none/full/instance) and give more control over block pin equivalnce specifications
 * VPR will produce a .route file even if the routing is illegal (aids debugging), however analysis results will not be produced unless `vpr --analsysis` is specified
 * VPR long arguments are now always prefixed by two dashes (e.g. `--route`) while short single-letter arguments are prefixed by a single dash (e.g. `-h`)
 * Improved logic optimization through using a recent 2018 version of ABC and new synthesis script
 * Significantly improved implementation quality (~14% smaller minimum routable channel widths, 32-42% reduced wirelength, 7-10% lower critical path delay)
 * Significantly reduced run-time (~5.5-6.3x faster) and memory usage (~3.3-5x lower)
 * Various other changes since VTR 7

### Fixed
 * FPGA Architecture file tags can be in arbitary orders
 * SDC command arguments can be in arbitary orders
 * Numerous other fixes since VTR 7

### Deprecated

### Removed
 * Classic VPR timing analyzer
 * IO channel distribution section of architecture file
