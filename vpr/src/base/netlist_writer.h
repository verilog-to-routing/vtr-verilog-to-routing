#pragma once

#include <memory>
#include <string>
#include "AnalysisDelayCalculator.h"

/**
 * @brief Writes out the post-synthesis implementation netlists in BLIF and Verilog formats,
 *        along with an SDF for delay annotations.
 *
 * Here, post-synthesis implementation netlist is the netlist as it appears after
 * routing (i.e. implementation is complete).
 *
 * All written filenames end in {basename}_post_synthesis.{fmt} where {basename} is the
 * basename argument and {fmt} is the file format (e.g. v, blif, sdf)
 */
void netlist_writer(const std::string basename, std::shared_ptr<const AnalysisDelayCalculator> delay_calc, t_analysis_opts opts);

/**
 * @brief Writes out the post implementation netlist in Verilog format.
 *        It has its top module ports merged into multi-bit ones.
 *
 * Here, post-synthesis implementation netlist is the netlist as it appears after
 * routing (i.e. implementation is complete).
 *
 * Written filename ends in {basename}_merged_post_implementation.v where {basename} is the
 * basename argument.
 */
void merged_netlist_writer(const std::string basename, std::shared_ptr<const AnalysisDelayCalculator> delay_calc, t_analysis_opts opts);
