#include "rr_rc_data.h"

t_rr_rc_data::t_rr_rc_data(float Rval, float Cval) noexcept
    : R(Rval)
    , C(Cval) {}

short find_create_rr_rc_data(const float R, const float C, std::vector<t_rr_rc_data>& rr_rc_data) {

    auto match = [&](const t_rr_rc_data& val) {
        return val.R == R
               && val.C == C;
    };

    //Just a linear search for now
    auto itr = std::find_if(rr_rc_data.begin(),
                            rr_rc_data.end(),
                            match);

    if (itr == rr_rc_data.end()) {
        //Note found -> create it
        rr_rc_data.emplace_back(R, C);

        itr = --rr_rc_data.end(); //Iterator to inserted value
    }

    return std::distance(rr_rc_data.begin(), itr);
}
