#include "rr_rc_data.h"
#include "globals.h"

t_rr_rc_data::t_rr_rc_data(float Rval, float Cval) noexcept
    : R(Rval)
    , C(Cval) {}

short find_create_rr_rc_data(const float R, const float C) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    auto match = [&](const t_rr_rc_data& val) {
        return val.R == R
               && val.C == C;
    };

    //Just a linear search for now
    auto itr = std::find_if(device_ctx.rr_rc_data.begin(),
                            device_ctx.rr_rc_data.end(),
                            match);

    if (itr == device_ctx.rr_rc_data.end()) {
        //Note found -> create it
        device_ctx.rr_rc_data.emplace_back(R, C);

        itr = --device_ctx.rr_rc_data.end(); //Iterator to inserted value
    }

    return std::distance(device_ctx.rr_rc_data.begin(), itr);
}
