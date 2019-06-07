#include "parameters.h"
#include "vtr_assert.h"

namespace fasm {

void Parameters::AddParameter(const std::string &eblif_parameter, const std::string &fasm_feature) {
    FeatureParameter fp;
    fp.width = FeatureWidth(fasm_feature);
    fp.feature = fasm_feature;
    features_.insert(std::make_pair(eblif_parameter, fp));
}

std::string Parameters::EmitFasmFeature(const std::string &eblif_parameter, const std::string &value) {
    std::ostringstream out;
    auto range = features_.equal_range(eblif_parameter);
    for(auto i = range.first; i != range.second; ++i) {
        // Parameter should have exactly the expected number of bits.
        const FeatureParameter &fp = i->second;
        if(value.size() != fp.width) {
            vpr_throw(VPR_ERROR_OTHER,
                      __FILE__, __LINE__, "When emitting FASM for parameter %s, expected width of %d got width of %d, value = \"%s\".",
                      eblif_parameter.c_str(), fp.width, value.size(), value.c_str());
        }
        VTR_ASSERT(value.size() == fp.width);
        out << fp.feature << "=" << fp.width << "'b" << value << std::endl;
    }
    return out.str();
}

size_t Parameters::FeatureWidth(const std::string &feature) const {
    size_t start_of_address = feature.rfind('[');
    size_t end_of_address = feature.rfind(']');

    if(start_of_address == std::string::npos) {
        VTR_ASSERT(end_of_address == std::string::npos);
        return 1;
    }

    VTR_ASSERT(end_of_address > start_of_address+1);

    auto address = feature.substr(start_of_address+1, end_of_address - start_of_address - 1);

    size_t address_split = address.find(':');

    if(address_split == std::string::npos) {
        return 1;
    }

    VTR_ASSERT(address_split > 0);
    VTR_ASSERT(address_split+1 < address.size());

    int high_slice = vtr::atoi(address.substr(0, address_split));
    int low_slice = vtr::atoi(address.substr(address_split+1));
    VTR_ASSERT(high_slice >= low_slice);

    return high_slice - low_slice + 1;
}

} // namespace fasm
