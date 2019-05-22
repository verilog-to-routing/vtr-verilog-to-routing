#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "netlist_writer.h"

namespace fasm {

// Utility class emit parameters from eblif
class Parameters {
 public:
  // Adds a parameter mapping between an eblif parameter (e.g. .param <param> <value>)
  // to FASM feature.
  void AddParameter(const std::string &eblif_parameter, const std::string &fasm_feature);

  // Return a FASM feature directive for the given parameter and value.
  std::string EmitFasmFeature(const std::string &eblif_parameter, const std::string &value);
 private:
  struct FeatureParameter {
      size_t width;
      std::string feature;
  };

  std::unordered_multimap<std::string, FeatureParameter> features_;

  size_t FeatureWidth(const std::string &feature) const;
};

} // namespace fasm

#endif  // PARAMETERS_H
