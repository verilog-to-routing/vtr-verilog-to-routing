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

template<typename T>
void fill_switch(T* switch_,
                 float R,
                 float Cin,
                 float Cout,
                 float Cinternal,
                 float Tdel,
                 float mux_trans_size,
                 float buf_size,
                 char* name,
                 SwitchType type) {
    switch_->R = R;
    switch_->Cin = Cin;
    switch_->Cout = Cout;
    switch_->Cinternal = Cinternal;
    switch_->Tdel = Tdel;
    switch_->mux_trans_size = mux_trans_size;
    switch_->buf_size = buf_size;
    switch_->name = name;
    switch_->set_type(type);
}

template<typename T1, typename T2>
void process_switches_array(DeviceResources::Device::Reader ar_,
                            std::set<std::tuple<int, bool>>& seen,
                            T1 switch_array,
                            std::vector<std::tuple<std::tuple<int, bool>, int>>& pips_models_) {
    fill_switch(&switch_array[T2(0)], 0, 0, 0, 0, 0, 0, 0,
                vtr::strdup("short"), SwitchType::SHORT);
    fill_switch(&switch_array[T2(1)], 0, 0, 0, 0, 0, 0, 0,
                vtr::strdup("generic"), SwitchType::MUX);

    const auto& pip_models = ar_.getPipTimings();
    float R, Cin, Cout, Cint, Tdel;
    std::string switch_name;
    SwitchType type;
    int id = 2;
    for (const auto& value : seen) {
        int timing_model_id;
        int mux_trans_size;
        bool buffered;
        std::tie(timing_model_id, buffered) = value;
        const auto& model = pip_models[timing_model_id];
        pips_models_.emplace_back(std::make_tuple(value, id));

        R = Cin = Cint = Cout = Tdel = 0.0;
        std::stringstream name;
        std::string mux_type_string = buffered ? "mux_" : "passGate_";
        name << mux_type_string;

        R = get_corner_value(model.getOutputResistance(), "slow", "min");
        name << "R" << std::scientific << R;

        Cin = get_corner_value(model.getInputCapacitance(), "slow", "min");
        name << "Cin" << std::scientific << Cin;

        Cout = get_corner_value(model.getOutputCapacitance(), "slow", "min");
        name << "Cout" << std::scientific << Cout;

        if (buffered) {
            Cint = get_corner_value(model.getInternalCapacitance(), "slow", "min");
            name << "Cinternal" << std::scientific << Cint;
        }

        Tdel = get_corner_value(model.getInternalDelay(), "slow", "min");
        name << "Tdel" << std::scientific << Tdel;

        switch_name = name.str();
        type = buffered ? SwitchType::MUX : SwitchType::PASS_GATE;
        mux_trans_size = buffered ? 1 : 0;

        fill_switch(&switch_array[T2(id)], R, Cin, Cout, Cint, Tdel,
                    mux_trans_size, 0, vtr::strdup(switch_name.c_str()), type);

        id++;
    }
}
