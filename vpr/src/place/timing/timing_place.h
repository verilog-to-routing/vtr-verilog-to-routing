/**
 * @file timing_place.h
 * @brief Interface used by the VPR placer to query information
 *        from the Tatum timing analyzer.
 *
 *   @class PlacerSetupSlacks
 *              Queries connection **RAW** setup slacks, which can
 *              range from negative to positive values. Also maps
 *              atom pin setup slacks to clb pin setup slacks.
 *   @class PlacerCriticalities
 *              Query connection criticalities, which are calculated
 *              based on the raw setup slacks and ranges from 0 to 1.
 *              Also maps atom pin crit. to clb pin crit.
 *   @class PlacerTimingCosts
 *              Hierarchical structure used by update_td_costs() to
 *              maintain the order of addition operation of float values
 *              (to avoid round-offs) while doing incremental updates.
 *
 * Calculating criticalities:
 *      All the raw setup slack values across a single clock domain are gathered
 *      and rated from the best to the worst in terms of criticalities. In order
 *      to calculate criticalities, all the slack values need to be non-negative.
 *      Hence, if the worst slack is negative, all the slack values are shifted
 *      by the value of the worst slack so that the value is at least 0. If the
 *      worst slack is positive, then no shift happens.
 *
 *      The best (shifted) slack (the most positive one) will have a criticality of 0.
 *      The worst (shifted) slack value will have a criticality of 1.
 *
 *      Criticalities are used to calculated timing costs for each connection.
 *      The formula is cost = delay * criticality.
 *
 *      For a more detailed description on how criticalities are calculated, see
 *      calc_relaxed_criticality() in `timing_util.cpp`.
 */



