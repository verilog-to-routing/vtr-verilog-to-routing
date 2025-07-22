#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    June 2025
 * @brief   Declaration of methods used to generate a report on the router
 *          lookahead heuristic used within VPR. This heuristic is used to guide
 *          the connection router to a solution faster. This report provides
 *          insight into the properties of the estimation it provides.
 */

// Forward declarations.
class RouterLookahead;
struct t_router_opts;

/**
 * @brief Generate a router lookahead report file for the given router lookahead.
 */
void generate_router_lookahead_report(const RouterLookahead* router_lookahead,
                                      const t_router_opts& router_opts);
