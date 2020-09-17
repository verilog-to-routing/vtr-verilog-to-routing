#ifndef NETLIST_WRITER_H
#define NETLIST_WRITER_H
#include <memory>
#include <string>
#include <sstream>

#include "vtr_logic.h"

#include "AnalysisDelayCalculator.h"

/**
 * @brief Writes out the post-synthesis implementation netlists in BLIF and Verilog formats,
 *        along with an SDF for delay annotations.
 *
 * All written filenames end in {basename}_post_synthesis.{fmt} where {basename} is the
 * basename argument and {fmt} is the file format (e.g. v, blif, sdf)
 */
void netlist_writer(const std::string basename, std::shared_ptr<const AnalysisDelayCalculator> delay_calc);

#endif
