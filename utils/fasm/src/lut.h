#ifndef LUT_H
#define LUT_H

#include "logic_vec.h"

namespace fasm {

// Utility class to create a LUT initialization.
class Lut {
 public:
  // Initialize an LUT of a given number of inputs.
  Lut(size_t num_inputs);

  // SetOutput sets the lut to output value when the inputs match.
  //
  // By default the output from the LUT is always false.
  void SetOutput(const std::vector<vtr::LogicValue> &inputs, vtr::LogicValue value);

  // Create a wire from input_pin to LUT output.
  // Also known as a route through LUT.
  //
  // input_pin must be less than num_inputs.
  void CreateWire(size_t input_pin);

  // Create a LUT with a constant output of value.
  void SetConstant(vtr::LogicValue value);

  // Return current LUT initialization.
  const LogicVec & table();
 private:
  size_t num_inputs_;
  LogicVec table_;
};

// Utility class that creates a FASM feature directive based on the FASM LUT definition.
struct LutOutputDefinition {
  // Definition should be of the format <feature>[<end_bit>:<start_bit].
  LutOutputDefinition(std::string definition);

  // Return a FASM feature directive for a wire from input specified to output.
  // Also known as a route through LUT.
  std::string CreateWire(int input) const;

  // Return a FASM feature directive for a constant LUT.
  std::string CreateConstant(vtr::LogicValue value) const;

  // Return a FASM feature directive for a LUT with the specified LUT initialization.
  std::string CreateInit(const LogicVec & table) const;

  // Base feature name.
  std::string fasm_feature;

  // Number of inputs to this LUT.
  int num_inputs;

  // First bit of the LUT INIT parameter.
  int start_bit;

  // Last bit of the LUT INIT parameter.
  int end_bit;
};

} // namespace fasm

#endif  // LUT_H
