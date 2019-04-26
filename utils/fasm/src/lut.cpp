#include "lut.h"

#include <sstream>

#include "vtr_assert.h"
#include "vpr_error.h"
#include "vtr_util.h"

namespace fasm {

Lut::Lut(size_t num_inputs) : num_inputs_(num_inputs), table_(1 << num_inputs, vtr::LogicValue::DONT_CARE) {}

void Lut::SetOutput(const std::vector<vtr::LogicValue> &inputs, vtr::LogicValue value) {
  VTR_ASSERT(inputs.size() == num_inputs_);
  std::vector<size_t> dont_care_inputs;
  dont_care_inputs.reserve(num_inputs_);

  for(size_t address = 0; address < table_.size(); ++address) {
    bool match = true;
    for(size_t input = 0; input < inputs.size(); ++input) {
      if(inputs[input] == vtr::LogicValue::TRUE && (address & (1 << input)) == 0) {
        match = false;
        break;
      } else if(inputs[input] == vtr::LogicValue::FALSE && (address & (1 << input)) != 0) {
        match = false;
        break;
      }
    }

    if(match) {
      VTR_ASSERT(table_[address] == vtr::LogicValue::DONT_CARE || table_[address] == value);
      table_[address] = value;
    }
  }
}

void Lut::CreateWire(size_t input_pin) {
  std::vector<vtr::LogicValue> inputs(num_inputs_, vtr::LogicValue::DONT_CARE);
  inputs[input_pin] = vtr::LogicValue::FALSE;
  SetOutput(inputs, vtr::LogicValue::FALSE);
  inputs[input_pin] = vtr::LogicValue::TRUE;
  SetOutput(inputs, vtr::LogicValue::TRUE);
}

void Lut::SetConstant(vtr::LogicValue value) {
  std::vector<vtr::LogicValue> inputs(num_inputs_, vtr::LogicValue::DONT_CARE);
  SetOutput(inputs, value);
}

const LogicVec & Lut::table() {
  // Make sure the entire table is defined.
  for(size_t address = 0; address < table_.size(); ++address) {
    if(table_[address] == vtr::LogicValue::DONT_CARE) {
      table_[address] = vtr::LogicValue::FALSE;
    }
  }

  return table_;
}

LutOutputDefinition::LutOutputDefinition(std::string definition) {
  // Parse LUT.INIT[63:0] into
  // fasm_feature = LUT.INIT
  // start_bit = 0
  // end_bit = 63
  // num_inputs = log2(end_bit-start_bit+1)

  size_t slice_start = definition.find_first_of('[');
  size_t slice = std::string::npos;
  size_t slice_end = std::string::npos;

  if(slice_start != std::string::npos) {
    slice = definition.find_first_of(':', slice_start);
  }
  if(slice != std::string::npos) {
    slice_end = definition.find_first_of(']');
  }

  if(slice_start == std::string::npos ||
      slice == std::string::npos ||
      slice_end == std::string::npos ||
      slice_start+1 > slice-1 ||
      slice+1 > slice_end-1) {
    vpr_throw(
        VPR_ERROR_OTHER, __FILE__, __LINE__,
        "Could not parse LUT definition %s",
        definition.c_str());
  }

  fasm_feature = definition.substr(0, slice_start);
  std::string end_bit_str = definition.substr(slice_start+1, (slice-1)-(slice_start+1)+1);
  std::string start_bit_str = definition.substr(slice+1, (slice_end-1)-(slice+1)+1);

  end_bit = vtr::atoi(end_bit_str);
  start_bit = vtr::atoi(start_bit_str);

  int width = end_bit - start_bit + 1;

  // If an exact power of two, only 1 bit will be set in width.
  if(width < 0 || __builtin_popcount(width) != 1) {
    vpr_throw(
        VPR_ERROR_OTHER, __FILE__, __LINE__,
        "Invalid LUT start_bit %d and end_bit %d, not a power of 2 width.",
        start_bit, end_bit);
  }

  // For exact power's of 2, ctz (count trailing zeros) is log2(width).
  num_inputs = __builtin_ctz(width);
}

std::string LutOutputDefinition::CreateWire(int input) const {
  Lut lut(num_inputs);
  lut.CreateWire(input);

  return CreateInit(lut.table());
}

std::string LutOutputDefinition::CreateConstant(vtr::LogicValue value) const {
  Lut lut(num_inputs);
  lut.SetConstant(value);
  return CreateInit(lut.table());
}

std::string LutOutputDefinition::CreateInit(const LogicVec & table) const {
  if(table.size() != (1u << num_inputs)) {
    vpr_throw(
        VPR_ERROR_OTHER, __FILE__, __LINE__,
        "LUT with %d inputs requires a INIT LogicVec of size %d, got %d",
        num_inputs, (1 << num_inputs), table.size());
  }
  std::stringstream ss;
  ss << fasm_feature << "[" << end_bit << ":" << start_bit << "]=" << table;

  return ss.str();
}

} // namespace fasm
