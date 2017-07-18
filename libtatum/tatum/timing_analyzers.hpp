#ifndef TATUM_TIMING_ANALYZERS_HPP
#define TATUM_TIMING_ANALYZERS_HPP

#include "timing_analyzers_fwd.hpp"

/** \file
 * Timing Analysis: Overview
 * ===========================
 * Timing analysis involves determining at what point in time (relative to some reference,
 * usually a clock) signals arrive at every point in a circuit. This is used to verify
 * that none of these signals will violate any constraints, ensuring the circuit will operate
 * correctly and reliably.
 *
 * The circuit is typically modelled as a directed graph (called a timing graph) where nodes
 * represent the 'pins' of elements in the circuit and edges the connectivity between them.
 * Delays through the circuit (e.g. along wires, or through combinational logic) are associated
 * with the edges.
 *
 * We are generally interested in whether signals arrive late (causing a setup
 * constraint violation) or arrive to early (causing a hold violation).  Typically long/slow
 * paths cause setup violations, and short/fast paths hold violations.  Violating a setup or
 * hold constraint can cause meta-stability in Flip-Flops putting the circuit into an
 * undetermined state for an indeterminate period of time (this is not good).
 *
 * To perform a completely accurate, non-pessemistic timing analysis would involve determining
 * exactly which set of state/inputs to the circuit trigger the worst case path. This requires
 * a dynamic analysis of the circuits behaviour and is prohibitively expensive to compute in
 * practice.
 *
 *
 * Static Timing Analysis (STA)
 * ------------------------------
 * Static Timing Analysis (STA), which this library implements, simplifies the problem somewhat
 * by ignoring the dynamic behaviour the circuit. That is, we assume all paths could be sensitized,
 * even if in practice they may be extremely rare or impossible to sensitize in practice. This
 * makes the result of our analysis pessemistic, but also makes the problem tractable.
 *
 * There are two approaches to performing STA: 'path-based' and 'block-based'.
 *
 * Under Path-Based Analysis (PBA) all paths in the circuit are analyzed. This provides
 * a more accurate (less-pessimistic) analysis than block-based approaches but can require
 * an exponential amount of time. In particular, circuit structures with re-convergent fanout
 * can exponentiallly increase the number of paths through the circuit which must be evaluated.
 *
 * To avoid this unpleasent behaviour, we implement 'block-based' STA. Under this formulation,
 * only the worst case values are kept at each node in the circuit. While this is more
 * pessimistic (any path passing through a node is now viewed as having the worst case delay
 * of any path through that node), it greatly reduces the computational complexity, allowing
 * STA to be perfomed in linear time.
 *
 * Arrival Time, Required Time & Slack
 * ---------------------------------------
 * When a Timing Analyzer performs timing analysis it is primarily calculating the following:
 *
 *    - Arrival Time: The time a signal actually arrived at a particular point in the circuit.
 *
 *    - Required Time: The time a signal should have arrived (was required) at a particular
 *                     to avoid violating a timing constraint.
 *
 *    - Slack: The difference between required and arrival times. This indicates how close
 *             a particular path/node is to violating its timing constraint.  A positive
 *             value indicates there is no violation, a negative value indicates there is
 *             a violation. The magnitude of the slack indicates by how much the constraint
 *             is passing/failing.  A value of zero indicates that the constraint is met
 *             exactly.
 *             TODO: Implement slack calculator
 *
 *
 * Calculating Arrival & Required Times
 * --------------------------------------
 * It is also useful to define the following collections of timing graph nodes:
 *    - Primary Inputs (PIs): circuit external input pins, Flip-Flop Q pins
 *    - Primary Outputs (POs): circuit external output pins, Flip-Flop D pins
 * Note that in the timing graph PIs have no fan-in, and POs have no fan-out.
 *
 * The arrival and required times are calculated at every node in the timing graph by walking
 * the timing graph in either a 'forward' or 'backward' direction. The following provide a
 * high-level description of the process.
 *
 * On the initial (forward) traversal, the graph is walked from PIs to POs to calculate arrival
 * time, performing the following:
 *      1) Initialize the arrival time of all PIs based on constraints (typically zero in the
 *         simplest case)
 *      2) Add the delay of each edge to the arrival time of the edge's driving node to
 *         to calculate the edge arrival time.
 *      3) At each downstream node calculate the max (setup) or min (hold) of all input
 *         edge arrival times, and store it as the node arrival time.
 *      4) Repeat (2)-(3) until all nodes have valid arrival times.
 *
 * On the second (backward) traversal, the graph is walked in reverse from POs to PIs, performing
 * the following:
 *      1) Initialize the required times of all POs based on constraints (typically target
 *         clock period for setup analysis)
 *      2) Subtract the delay of each edge to the required time of the edge's sink node to
 *         to calculate the edge required time.
 *      3) At each upstream node calculate the min (setup) or max (hold) of all input
 *         edge required times, and store it as the node required time.
 *      4) Repeat (2)-(3) until all nodes have valid required times.
 *
 * Clock Skew
 * ------------
 * In a real system the clocks which launch signals at the PIs and capture them at POs may not
 * all arrive at the same instance in time.  This difference is known as 'skew'.
 *
 * Skew can be modled by adjusting the initialized PI arrival times to reflect when the clock
 * signal actually reaches the node.  Similarily the PO required times can also be adjusted.
 *
 * Multi-clock Analysis
 * ----------------------
 * The previous discussion has focused primarily on single-clock STA. In a multi-clock analysis
 * transfers between clock domains need to be handled (e.g. if the launch and capture clocks are
 * different). This is typically handled by identifying the worst-case alignment between all pairs
 * of clocks. This worst case value then becomes the constraint between the two clocks.
 *
 * To perform a multi-clock analysis the paths between different clocks need to be considered with
 * the identified constraint applied.  This can be handled in two ways either:
 *      a) Performing multiple single-clock analysis passes (one for each pair of clocks), or
 *      b) Perform a single analysis but track multiple arrival/required times (a unique one for
 *         each clock).
 *
 * Approach (b) turns out to be more efficient in practice, since it only requires a single traversal
 * of the timing graph. The combined values {clock, arrival time, required time} (and potentially other
 * info) are typically combined into a single 'Tag'.  As a result there may be multiple tags stored
 * at each node in the timing graph.
 */

 /* XXX TODO: these features haven't yet been implemented!
 * ======================================================
 *
 * Derating & Pesimism Reduction Techniques
 * ------------------------------------------
 *
 * Unlike the previous discussion (which assumed constant delay values), in reality circuit delays
 * varry based on a variety of different parameters.  To generate a correct (i.e. pessimistic and not
 * optimistic) analysis this means we must often choose a highly pessemistic value for this single
 * delay. In order to recovering some of this pessimism requires modeling more details of the system.
 *
 * Slew & Rise/Fall
 * ------------------
 * Two of the key parameters that the single delay model does not account for are the impact of:
 *     - Signals 'slew rate' (e.g. signal transition time from low to high), which can effect
 *       the delay through a logic gate
 *     - Signal direction (i.e. is a logic gate's output rising or falling). In CMOS
 *       technologies different transitors of different types are activated depending on the
 *       direction of the output signal (e.g. NMOS pull-down vs PMOS pull-up)
 *
 * Derating
 * ----------
 * Another challenge in modern CMOS technologies is the presence of On-Chip Variation (OCV),
 * identically design structures (e.g. transistors, wires) may have different performance
 * due to manufacturing variation. To capture this behaviour one approach is to apply a
 * derate to delays on different paths in the circuit.
 *
 * In the typical approach applies incrementally more advanced derating, focusing firstly
 * on the most significant sources of variation. A typical progressing is to apply derating to:
 *      1) Clock Path
 *          Since clocks often span large portions of the chip they can be subject to large
 *          variation. To ensure a pessemistic analysis an late (early) derate is applied to
 *          clock launch paths, and a early (launch) derate to clock capture paths for a setup
 *          (hold) analysis.
 *
 *      2) Data Path
 *          Early (late) derates can also be applied to data paths to account for their variation
 *          during setup (hold) analysis.
 *          Note that data paths tend to be more localized to clock networks so the impact of
 *          OCV tends to be smaller.
 *
 * The simplest form of derating is a fixed derate applied to every delay, this can be extended
 * to different delays for different types of circuit elements (wires vs cells, individual cells
 * etc.).
 *
 * It turns out that fixed derates are overly pessemistic, since it is unlikely (particularliy on
 * long paths) that random variation will always go in one direction.  Improved forms of derating
 * take into account the length (depth) of a path, often using a (user specified) table of derates
 * which falls off (derates less) along deeper paths.  It is also possible for the derate to take
 * into account the physical locality/spread of a path.
 *
 * Common Clock Pessimism Removal (CCPR)
 * ---------------------------------------
 * Note: Names for this vary, including: Clock Reconvergence Pessimism Removal (CRPR),
 *       Common Path Pessimism Removal (CPPR), and likely others.
 *
 * Derating can also introduce pessimism when applied to clock networks if the launch and
 * capture paths share some common portion of the clock network. Specifically, using
 * early/late derates on the launch/capture paths of a clock network may model a scenario
 * which is physically impossible to occur:  the shared portion of the clock path cannot
 * be both early and late at the same time.
 *
 * CCPR does not directly remove this effect, but instead calculates a 'credit' which is
 * added back to the final path to counter-act this extra pessimism.
 */

/*
 * IMPLEMENTATION NOTES
 * ====================
 *
 * All the timing analyzers included here are pure abstract classes.
 * They should all have pure virtual functions and store NO data members.
 *
 * We use multiple (virtual) inheritance to define the SetupHoldTimingAnalyzer
 * class, allowing it to be cleanly substituted for any SetupTimingAnalyzer
 * or HoldTimingAnalyzer.
 *
 * Note also that we are using the NVF (Non-Virtual Interface) pattern, so
 * any public member functions should delegate their work to an appropriate
 * protected virtual member function.
 */

#include "analyzers/TimingAnalyzer.hpp"
#include "analyzers/SetupTimingAnalyzer.hpp"
#include "analyzers/HoldTimingAnalyzer.hpp"
#include "analyzers/SetupHoldTimingAnalyzer.hpp"

#endif
