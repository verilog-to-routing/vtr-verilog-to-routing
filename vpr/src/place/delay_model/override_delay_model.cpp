
#include "override_delay_model.h"

#include "compute_delta_delays_utils.h"
#include "physical_types_util.h"

#ifdef VTR_ENABLE_CAPNPROTO
#    include "capnp/serialize.h"
#    include "place_delay_model.capnp.h"
#    include "ndmatrix_serdes.h"
#    include "mmap_file.h"
#    include "serdes_utils.h"
#endif  // VTR_ENABLE_CAPNPROTO

void OverrideDelayModel::compute(RouterDelayProfiler& route_profiler,
                                 const t_placer_opts& placer_opts,
                                 const t_router_opts& router_opts,
                                 int longest_length) {
    auto delays = compute_delta_delay_model(route_profiler,
                                            placer_opts,
                                            router_opts,
                                            /*measure_directconnect=*/false,
                                            longest_length,
                                            is_flat_);

    base_delay_model_ = std::make_unique<DeltaDelayModel>(cross_layer_delay_, delays, false);

    compute_override_delay_model_(route_profiler, router_opts);
}

void OverrideDelayModel::compute_override_delay_model_(RouterDelayProfiler& route_profiler,
                                                       const t_router_opts& router_opts) {
    const auto& device_ctx = g_vpr_ctx.device();
    t_router_opts router_opts2 = router_opts;
    router_opts2.astar_fac = 0.f;
    router_opts2.astar_offset = 0.f;

    // Look at all the direct connections that exist, and add overrides to delay model
    for (int idirect = 0; idirect < (int)device_ctx.arch->directs.size(); ++idirect) {
        const t_direct_inf* direct = &device_ctx.arch->directs[idirect];

        InstPort from_port = parse_inst_port(direct->from_pin);
        InstPort to_port = parse_inst_port(direct->to_pin);

        t_physical_tile_type_ptr from_type = find_tile_type_by_name(from_port.instance_name(), device_ctx.physical_tile_types);
        t_physical_tile_type_ptr to_type = find_tile_type_by_name(to_port.instance_name(), device_ctx.physical_tile_types);

        int num_conns = from_port.port_high_index() - from_port.port_low_index() + 1;
        VTR_ASSERT_MSG(num_conns == to_port.port_high_index() - to_port.port_low_index() + 1, "Directs must have the same size to/from");

        //We now walk through all the connections associated with the current direct specification, measure
        //their delay and specify that value as an override in the delay model.
        //
        //Note that we need to check every connection in the direct to cover the case where the pins are not
        //equivalent.
        //
        //However, if the from/to ports are equivalent we could end up sampling the same RR SOURCE/SINK
        //paths multiple times (wasting CPU time) -- we avoid this by recording the sampled paths in
        //sampled_rr_pairs and skipping them if they occur multiple times.
        int missing_instances = 0;
        int missing_paths = 0;
        std::set<std::pair<RRNodeId, RRNodeId>> sampled_rr_pairs;
        for (int iconn = 0; iconn < num_conns; ++iconn) {
            //Find the associated pins
            int from_pin = from_type->find_pin(from_port.port_name(), from_port.port_low_index() + iconn);
            int to_pin = to_type->find_pin(to_port.port_name(), to_port.port_low_index() + iconn);

            VTR_ASSERT(from_pin != OPEN);
            VTR_ASSERT(to_pin != OPEN);

            int from_pin_class = from_type->find_pin_class(from_port.port_name(), from_port.port_low_index() + iconn, DRIVER);
            VTR_ASSERT(from_pin_class != OPEN);

            int to_pin_class = to_type->find_pin_class(to_port.port_name(), to_port.port_low_index() + iconn, RECEIVER);
            VTR_ASSERT(to_pin_class != OPEN);

            bool found_sample_points;
            RRNodeId src_rr, sink_rr;
            found_sample_points = find_direct_connect_sample_locations(direct, from_type, from_pin, from_pin_class, to_type, to_pin, to_pin_class, src_rr, sink_rr);

            if (!found_sample_points) {
                ++missing_instances;
                continue;
            }

            //If some of the source/sink ports are logically equivalent we may have already
            //sampled the associated source/sink pair and don't need to do so again
            if (sampled_rr_pairs.count({src_rr, sink_rr})) continue;

            float direct_connect_delay = std::numeric_limits<float>::quiet_NaN();
            bool found_routing_path = route_profiler.calculate_delay(src_rr, sink_rr, router_opts2, &direct_connect_delay);

            if (found_routing_path) {
                set_delay_override(from_type->index, from_pin_class, to_type->index, to_pin_class, direct->x_offset, direct->y_offset, direct_connect_delay);
            } else {
                ++missing_paths;
            }

            //Record that we've sampled this pair of source and sink nodes
            sampled_rr_pairs.insert({src_rr, sink_rr});
        }

        VTR_LOGV_WARN(missing_instances > 0, "Found no delta delay for %d bits of inter-block direct connect '%s' (no instances of this direct found)\n", missing_instances, direct->name.c_str());
        VTR_LOGV_WARN(missing_paths > 0, "Found no delta delay for %d bits of inter-block direct connect '%s' (no routing path found)\n", missing_paths, direct->name.c_str());
    }
}

const DeltaDelayModel* OverrideDelayModel::base_delay_model() const {
    return base_delay_model_.get();
}

float OverrideDelayModel::delay(const t_physical_tile_loc& from_loc, int from_pin, const t_physical_tile_loc& to_loc, int to_pin) const {
    // First check to if there is an override delay value
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& grid = device_ctx.grid;

    t_physical_tile_type_ptr from_type_ptr = grid.get_physical_type(from_loc);
    t_physical_tile_type_ptr to_type_ptr = grid.get_physical_type(to_loc);

    t_override override_key;
    override_key.from_type = from_type_ptr->index;
    override_key.from_class = from_type_ptr->pin_class[from_pin];
    override_key.to_type = to_type_ptr->index;
    override_key.to_class = to_type_ptr->pin_class[to_pin];

    //Delay overrides may be different for +/- delta so do not use
    //an absolute delta for the look-up
    override_key.delta_x = to_loc.x - from_loc.x;
    override_key.delta_y = to_loc.y - from_loc.y;

    float delay_val = std::numeric_limits<float>::quiet_NaN();
    auto override_iter = delay_overrides_.find(override_key);
    if (override_iter != delay_overrides_.end()) {
        //Found an override
        delay_val = override_iter->second;
    } else {
        //Fall back to the base delay model if no override was found
        delay_val = base_delay_model_->delay(from_loc, from_pin, to_loc, to_pin);
    }

    return delay_val;
}

void OverrideDelayModel::set_delay_override(int from_type, int from_class, int to_type, int to_class, int delta_x, int delta_y, float delay_val) {
    t_override override_key;
    override_key.from_type = from_type;
    override_key.from_class = from_class;
    override_key.to_type = to_type;
    override_key.to_class = to_class;
    override_key.delta_x = delta_x;
    override_key.delta_y = delta_y;

    auto res = delay_overrides_.insert(std::make_pair(override_key, delay_val));
    if (!res.second) {                 //Key already exists
        res.first->second = delay_val; //Overwrite existing delay
    }
}

void OverrideDelayModel::dump_echo(std::string filepath) const {
    base_delay_model_->dump_echo(filepath);

    FILE* f = vtr::fopen(filepath.c_str(), "a");

    fprintf(f, "\n");
    fprintf(f, "# Delay Overrides\n");
    auto& device_ctx = g_vpr_ctx.device();
    for (auto kv : delay_overrides_) {
        auto override_key = kv.first;
        float delay_val = kv.second;
        fprintf(f, "from_type: %s to_type: %s from_pin_class: %d to_pin_class: %d delta_x: %d delta_y: %d -> delay: %g\n",
                device_ctx.physical_tile_types[override_key.from_type].name.c_str(),
                device_ctx.physical_tile_types[override_key.to_type].name.c_str(),
                override_key.from_class,
                override_key.to_class,
                override_key.delta_x,
                override_key.delta_y,
                delay_val);
    }

    vtr::fclose(f);
}

float OverrideDelayModel::get_delay_override(int from_type, int from_class, int to_type, int to_class, int delta_x, int delta_y) const {
    t_override key;
    key.from_type = from_type;
    key.from_class = from_class;
    key.to_type = to_type;
    key.to_class = to_class;
    key.delta_x = delta_x;
    key.delta_y = delta_y;

    auto iter = delay_overrides_.find(key);
    if (iter == delay_overrides_.end()) {
        VPR_THROW(VPR_ERROR_PLACE, "Key not found.");
    }
    return iter->second;
}

void OverrideDelayModel::set_base_delay_model(std::unique_ptr<DeltaDelayModel> base_delay_model_obj) {
    base_delay_model_ = std::move(base_delay_model_obj);
}

void OverrideDelayModel::read(const std::string& file) {
#ifndef VTR_ENABLE_CAPNPROTO
    (void)file;
    VPR_THROW(VPR_ERROR_PLACE,
              "OverrideDelayModel::read is disabled because VTR_ENABLE_CAPNPROTO=OFF. "
              "Re-compile with CMake option VTR_ENABLE_CAPNPROTO=ON to enable.");
#else
    MmapFile f(file);

    /* Increase reader limit to 1G words to allow for large files. */
    ::capnp::ReaderOptions opts = default_large_capnp_opts();
    ::capnp::FlatArrayMessageReader reader(f.getData(), opts);

    auto toFloat = [](float* out, const VprFloatEntry::Reader& in) -> void {
        *out = in.getValue();
    };

    vtr::NdMatrix<float, 4> delays;
    auto model = reader.getRoot<VprOverrideDelayModel>();
    ToNdMatrix<4, VprFloatEntry, float>(&delays, model.getDelays(), toFloat);

    base_delay_model_ = std::make_unique<DeltaDelayModel>(cross_layer_delay_, delays, is_flat_);

    // Reading non-scalar capnproto fields is roughly equivalent to using
    // a std::vector of the field type.  Actual type is capnp::List<X>::Reader.
    auto overrides = model.getDelayOverrides();
    std::vector<std::pair<t_override, float> > overrides_arr(overrides.size());
    for (size_t i = 0; i < overrides.size(); ++i) {
        const auto& elem = overrides[i];
        overrides_arr[i].first.from_type = elem.getFromType();
        overrides_arr[i].first.to_type = elem.getToType();
        overrides_arr[i].first.from_class = elem.getFromClass();
        overrides_arr[i].first.to_class = elem.getToClass();
        overrides_arr[i].first.delta_x = elem.getDeltaX();
        overrides_arr[i].first.delta_y = elem.getDeltaY();

        overrides_arr[i].second = elem.getDelay();
    }

    delay_overrides_ = vtr::make_flat_map2(std::move(overrides_arr));
#endif
}

void OverrideDelayModel::write(const std::string& file) const {
#ifndef VTR_ENABLE_CAPNPROTO
    (void)file;
    VPR_THROW(VPR_ERROR_PLACE,
              "OverrideDelayModel::write is disabled because VTR_ENABLE_CAPNPROTO=OFF. "
              "Re-compile with CMake option VTR_ENABLE_CAPNPROTO=ON to enable.");
#else
    ::capnp::MallocMessageBuilder builder;
    auto model = builder.initRoot<VprOverrideDelayModel>();

    auto fromFloat = [](VprFloatEntry::Builder* out, const float& in) -> void {
        out->setValue(in);
    };

    auto delays = model.getDelays();
    FromNdMatrix<4, VprFloatEntry, float>(&delays, base_delay_model_->delays(), fromFloat);

    // Non-scalar capnproto fields should be first initialized with
    // init<field  name>(count), and then accessed from the returned
    // std::vector-like Builder object (specifically capnp::List<X>::Builder).
    auto overrides = model.initDelayOverrides(delay_overrides_.size());
    auto dst_iter = overrides.begin();
    for (const auto& src : delay_overrides_) {
        auto elem = *dst_iter++;
        elem.setFromType(src.first.from_type);
        elem.setToType(src.first.to_type);
        elem.setFromClass(src.first.from_class);
        elem.setToClass(src.first.to_class);
        elem.setDeltaX(src.first.delta_x);
        elem.setDeltaY(src.first.delta_y);

        elem.setDelay(src.second);
    }

    writeMessageToFile(file, &builder);
#endif
}

