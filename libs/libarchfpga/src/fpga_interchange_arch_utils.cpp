#include "fpga_interchange_arch_utils.h"

/****************** Utility functions ******************/

/**
 * @brief The FPGA interchange timing model includes three different corners (min, typ and max) for each of the two
 * speed_models (slow and fast).
 *
 * Timing data can be found on PIPs, nodes, site pins and bel pins.
 * This function retrieves the timing value based on the wanted speed model and the wanted corner.
 *
 * Corner model is considered valid if at least one configuration is set.
 * In that case this value shall be returned.
 *
 * More information on the FPGA Interchange timing model can be found here:
 *   - https://github.com/chipsalliance/fpga-interchange-schema/blob/main/interchange/DeviceResources.capnp
 */

float get_corner_value(DeviceResources::Device::CornerModel::Reader model, const char* speed_model, const char* value) {
    bool slow_model = std::string(speed_model) == std::string("slow");
    bool fast_model = std::string(speed_model) == std::string("fast");

    bool min_corner = std::string(value) == std::string("min");
    bool typ_corner = std::string(value) == std::string("typ");
    bool max_corner = std::string(value) == std::string("max");

    if (!slow_model && !fast_model) {
        archfpga_throw("", __LINE__, "Wrong speed model `%s`. Expected `slow` or `fast`\n", speed_model);
    }

    if (!min_corner && !typ_corner && !max_corner) {
        archfpga_throw("", __LINE__, "Wrong corner model `%s`. Expected `min`, `typ` or `max`\n", value);
    }

    bool has_fast = model.getFast().hasFast();
    bool has_slow = model.getSlow().hasSlow();

    if ((slow_model || (fast_model && !has_fast)) && has_slow) {
        auto half = model.getSlow().getSlow();
        if (min_corner && half.getMin().isMin()) {
            return half.getMin().getMin();
        } else if (typ_corner && half.getTyp().isTyp()) {
            return half.getTyp().getTyp();
        } else if (max_corner && half.getMax().isMax()) {
            return half.getMax().getMax();
        } else {
            if (half.getMin().isMin()) {
                return half.getMin().getMin();
            } else if (half.getTyp().isTyp()) {
                return half.getTyp().getTyp();
            } else if (half.getMax().isMax()) {
                return half.getMax().getMax();
            } else {
                archfpga_throw("", __LINE__, "Invalid speed model %s. No value found!\n", speed_model);
            }
        }
    } else if ((fast_model || slow_model) && has_fast) {
        auto half = model.getFast().getFast();
        if (min_corner && half.getMin().isMin()) {
            return half.getMin().getMin();
        } else if (typ_corner && half.getTyp().isTyp()) {
            return half.getTyp().getTyp();
        } else if (max_corner && half.getMax().isMax()) {
            return half.getMax().getMax();
        } else {
            if (half.getMin().isMin()) {
                return half.getMin().getMin();
            } else if (half.getTyp().isTyp()) {
                return half.getTyp().getTyp();
            } else if (half.getMax().isMax()) {
                return half.getMax().getMax();
            } else {
                archfpga_throw("", __LINE__, "Invalid speed model %s. No value found!\n", speed_model);
            }
        }
    }
    return 0.;
}
