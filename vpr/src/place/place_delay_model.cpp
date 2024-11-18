/**
 * @file place_delay_model.cpp
 * @brief This file implements all the class methods and individual
 *        routines related to the placer delay model.
 */

#include <queue>
#include "place_delay_model.h"
#include "globals.h"
#include "router_lookahead_map.h"
#include "rr_graph2.h"

#include "timing_place_lookup.h"
#include "placer_state.h"

#include "vtr_log.h"
#include "vtr_math.h"
#include "vpr_error.h"

#ifdef VTR_ENABLE_CAPNPROTO
#    include "capnp/serialize.h"
#    include "place_delay_model.capnp.h"
#    include "ndmatrix_serdes.h"
#    include "mmap_file.h"
#    include "serdes_utils.h"
#endif /* VTR_ENABLE_CAPNPROTO */

///@brief DeltaDelayModel methods.
float DeltaDelayModel::delay(const t_physical_tile_loc& from_loc, int /*from_pin*/, const t_physical_tile_loc& to_loc, int /*to_pin*/) const {
    int delta_x = std::abs(from_loc.x - to_loc.x);
    int delta_y = std::abs(from_loc.y - to_loc.y);

    return delays_[from_loc.layer_num][to_loc.layer_num][delta_x][delta_y];
}

void DeltaDelayModel::dump_echo(std::string filepath) const {
    FILE* f = vtr::fopen(filepath.c_str(), "w");
    fprintf(f, "         ");
    for (size_t from_layer_num = 0; from_layer_num < delays_.dim_size(0); ++from_layer_num) {
        for (size_t to_layer_num = 0; to_layer_num < delays_.dim_size(1); ++to_layer_num) {
            fprintf(f, " %9zu", from_layer_num);
            fprintf(f, "\n");
            for (size_t dx = 0; dx < delays_.dim_size(2); ++dx) {
                fprintf(f, " %9zu", dx);
            }
            fprintf(f, "\n");
            for (size_t dy = 0; dy < delays_.dim_size(3); ++dy) {
                fprintf(f, "%9zu", dy);
                for (size_t dx = 0; dx < delays_.dim_size(2); ++dx) {
                    fprintf(f, " %9.2e", delays_[from_layer_num][to_layer_num][dx][dy]);
                }
                fprintf(f, "\n");
            }
        }
    }
    vtr::fclose(f);
}

const DeltaDelayModel* OverrideDelayModel::base_delay_model() const {
    return base_delay_model_.get();
}

///@brief OverrideDelayModel methods.
float OverrideDelayModel::delay(const t_physical_tile_loc& from_loc, int from_pin, const t_physical_tile_loc& to_loc, int to_pin) const {
    //First check to if there is an override delay value
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

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

float SimpleDelayModel::delay(const t_physical_tile_loc& from_loc, int /*from_pin*/, const t_physical_tile_loc& to_loc, int /*to_pin*/) const {
    int delta_x = std::abs(from_loc.x - to_loc.x);
    int delta_y = std::abs(from_loc.y - to_loc.y);

    int from_tile_idx = g_vpr_ctx.device().grid.get_physical_type(from_loc)->index;
    return delays_[from_tile_idx][from_loc.layer_num][to_loc.layer_num][delta_x][delta_y];
}

/**
 * When writing capnp targetted serialization, always allow compilation when
 * VTR_ENABLE_CAPNPROTO=OFF. Generally this means throwing an exception instead.
 */
#ifndef VTR_ENABLE_CAPNPROTO

#    define DISABLE_ERROR                              \
        "is disable because VTR_ENABLE_CAPNPROTO=OFF." \
        "Re-compile with CMake option VTR_ENABLE_CAPNPROTO=ON to enable."

void DeltaDelayModel::read(const std::string& /*file*/) {
    VPR_THROW(VPR_ERROR_PLACE, "DeltaDelayModel::read " DISABLE_ERROR);
}

void DeltaDelayModel::write(const std::string& /*file*/) const {
    VPR_THROW(VPR_ERROR_PLACE, "DeltaDelayModel::write " DISABLE_ERROR);
}

void OverrideDelayModel::read(const std::string& /*file*/) {
    VPR_THROW(VPR_ERROR_PLACE, "OverrideDelayModel::read " DISABLE_ERROR);
}

void OverrideDelayModel::write(const std::string& /*file*/) const {
    VPR_THROW(VPR_ERROR_PLACE, "OverrideDelayModel::write " DISABLE_ERROR);
}

#else /* VTR_ENABLE_CAPNPROTO */

static void ToFloat(float* out, const VprFloatEntry::Reader& in) {
    // Getting a scalar field is always "get<field name>()".
    *out = in.getValue();
}

static void FromFloat(VprFloatEntry::Builder* out, const float& in) {
    // Setting a scalar field is always "set<field name>(value)".
    out->setValue(in);
}

void DeltaDelayModel::read(const std::string& file) {
    // MmapFile object creates an mmap of the specified path, and will munmap
    // when the object leaves scope.
    MmapFile f(file);

    /* Increase reader limit to 1G words to allow for large files. */
    ::capnp::ReaderOptions opts = default_large_capnp_opts();

    // FlatArrayMessageReader is used to read the message from the data array
    // provided by MmapFile.
    ::capnp::FlatArrayMessageReader reader(f.getData(), opts);

    // When reading capnproto files the Reader object to use is named
    // <schema name>::Reader.
    //
    // Initially this object is an empty VprDeltaDelayModel.
    VprDeltaDelayModel::Reader model;

    // The reader.getRoot performs a cast from the generic capnproto to fit
    // with the specified schema.
    //
    // Note that capnproto does not validate that the incoming data matches the
    // schema.  If this property is required, some form of check would be
    // required.
    model = reader.getRoot<VprDeltaDelayModel>();

    // ToNdMatrix is a generic function for converting a Matrix capnproto
    // to a vtr::NdMatrix.
    //
    // The use must supply the matrix dimension (2 in this case), the source
    // capnproto type (VprFloatEntry),
    // target C++ type (flat), and a function to convert from the source capnproto
    // type to the target C++ type (ToFloat).
    //
    // The second argument should be of type Matrix<X>::Reader where X is the
    // capnproto element type.
    ToNdMatrix<4, VprFloatEntry, float>(&delays_, model.getDelays(), ToFloat);
}

void DeltaDelayModel::write(const std::string& file) const {
    // MallocMessageBuilder object is the generate capnproto message builder,
    // using malloc for buffer allocation.
    ::capnp::MallocMessageBuilder builder;

    // initRoot<X> returns a X::Builder object that can be used to set the
    // fields in the message.
    auto model = builder.initRoot<VprDeltaDelayModel>();

    // FromNdMatrix is a generic function for converting a vtr::NdMatrix to a
    // Matrix message.  It is the mirror function of ToNdMatrix described in
    // read above.
    auto delay_values = model.getDelays();
    FromNdMatrix<4, VprFloatEntry, float>(&delay_values, delays_, FromFloat);

    // writeMessageToFile writes message to the specified file.
    writeMessageToFile(file, &builder);
}

void OverrideDelayModel::read(const std::string& file) {
    MmapFile f(file);

    /* Increase reader limit to 1G words to allow for large files. */
    ::capnp::ReaderOptions opts = default_large_capnp_opts();
    ::capnp::FlatArrayMessageReader reader(f.getData(), opts);

    vtr::NdMatrix<float, 4> delays;
    auto model = reader.getRoot<VprOverrideDelayModel>();
    ToNdMatrix<4, VprFloatEntry, float>(&delays, model.getDelays(), ToFloat);

    base_delay_model_ = std::make_unique<DeltaDelayModel>(cross_layer_delay_, delays, is_flat_);

    // Reading non-scalar capnproto fields is roughly equivilant to using
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
}

void OverrideDelayModel::write(const std::string& file) const {
    ::capnp::MallocMessageBuilder builder;
    auto model = builder.initRoot<VprOverrideDelayModel>();

    auto delays = model.getDelays();
    FromNdMatrix<4, VprFloatEntry, float>(&delays, base_delay_model_->delays(), FromFloat);

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
}

#endif

///@brief Initialize the placer delay model.
std::unique_ptr<PlaceDelayModel> alloc_lookups_and_delay_model(const Netlist<>& net_list,
                                                               t_chan_width_dist chan_width_dist,
                                                               const t_placer_opts& placer_opts,
                                                               const t_router_opts& router_opts,
                                                               t_det_routing_arch* det_routing_arch,
                                                               std::vector<t_segment_inf>& segment_inf,
                                                               const std::vector<t_direct_inf>& directs,
                                                               bool is_flat) {
    return compute_place_delay_model(placer_opts,
                                     router_opts,
                                     net_list,
                                     det_routing_arch,
                                     segment_inf,
                                     chan_width_dist,
                                     directs,
                                     is_flat);
}

/**
 * @brief Returns the delay of one point to point connection.
 *
 * Only estimate delay for signals routed through the inter-block routing network.
 * TODO: Do how should we compute the delay for globals. "Global signals are assumed to have zero delay."
 */
float comp_td_single_connection_delay(const PlaceDelayModel* delay_model,
                                      const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs,
                                      ClusterNetId net_id,
                                      int ipin) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    float delay_source_to_sink = 0.;

    if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
        ClusterPinId source_pin = cluster_ctx.clb_nlist.net_driver(net_id);
        ClusterPinId sink_pin = cluster_ctx.clb_nlist.net_pin(net_id, ipin);

        ClusterBlockId source_block = cluster_ctx.clb_nlist.pin_block(source_pin);
        ClusterBlockId sink_block = cluster_ctx.clb_nlist.pin_block(sink_pin);

        int source_block_ipin = cluster_ctx.clb_nlist.pin_logical_index(source_pin);
        int sink_block_ipin = cluster_ctx.clb_nlist.pin_logical_index(sink_pin);

        t_pl_loc source_block_loc = block_locs[source_block].loc;
        t_pl_loc sink_block_loc = block_locs[sink_block].loc;

        /**
         * This heuristic only considers delta_x and delta_y, a much better
         * heuristic would be to to create a more comprehensive lookup table.
         *
         * In particular this approach does not accurately capture the effect
         * of fast carry-chain connections.
         */
        delay_source_to_sink = delay_model->delay({source_block_loc.x, source_block_loc.y, source_block_loc.layer}, source_block_ipin,
                                                  {sink_block_loc.x, sink_block_loc.y, sink_block_loc.layer}, sink_block_ipin);
        if (delay_source_to_sink < 0) {
            VPR_ERROR(VPR_ERROR_PLACE,
                      "in comp_td_single_connection_delay: Bad delay_source_to_sink value %g from %s (at %d,%d,%d) to %s (at %d,%d,%d)\n"
                      "in comp_td_single_connection_delay: Delay is less than 0\n",
                      block_type_pin_index_to_name(physical_tile_type(source_block_loc), source_block_ipin, false).c_str(),
                      source_block_loc.x, source_block_loc.y, source_block_loc.layer,
                      block_type_pin_index_to_name(physical_tile_type(sink_block_loc), sink_block_ipin, false).c_str(),
                      sink_block_loc.x, sink_block_loc.y, sink_block_loc.layer,
                      delay_source_to_sink);
        }
    }

    return (delay_source_to_sink);
}

///@brief Recompute all point to point delays, updating `connection_delay` matrix.
void comp_td_connection_delays(const PlaceDelayModel* delay_model,
                               PlacerState& placer_state) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& p_timing_ctx = placer_state.mutable_timing();
    auto& block_locs = placer_state.block_locs();
    auto& connection_delay = p_timing_ctx.connection_delay;

    for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) {
        for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ++ipin) {
            connection_delay[net_id][ipin] = comp_td_single_connection_delay(delay_model, block_locs, net_id, ipin);
        }
    }
}
