
#include "simple_delay_model.h"

#ifdef VTR_ENABLE_CAPNPROTO
#    include "capnp/serialize.h"
#    include "place_delay_model.capnp.h"
#    include "ndmatrix_serdes.h"
#    include "mmap_file.h"
#    include "serdes_utils.h"
#endif  // VTR_ENABLE_CAPNPROTO

void SimpleDelayModel::compute(RouterDelayProfiler& route_profiler,
                               const t_placer_opts& /*placer_opts*/,
                               const t_router_opts& /*router_opts*/,
                               int /*longest_length*/) {
    const auto& grid = g_vpr_ctx.device().grid;
    const size_t num_physical_tile_types = g_vpr_ctx.device().physical_tile_types.size();
    const size_t num_layers = grid.get_num_layers();

    // Initializing the delay matrix to [num_physical_types][num_layers][num_layers][width][height]
    // The second index related to the layer that the source location is on and the third index is for the sink layer
    delays_ = vtr::NdMatrix<float, 5>({num_physical_tile_types,
                                       num_layers,
                                       num_layers,
                                       grid.width(),
                                       grid.height()});

    for (size_t physical_tile_type_idx = 0; physical_tile_type_idx < num_physical_tile_types; ++physical_tile_type_idx) {
        for (size_t from_layer = 0; from_layer < num_layers; ++from_layer) {
            for (size_t to_layer = 0; to_layer < num_layers; ++to_layer) {
                for (size_t dx = 0; dx < grid.width(); ++dx) {
                    for (size_t dy = 0; dy < grid.height(); ++dy) {
                        float min_delay = route_profiler.get_min_delay(physical_tile_type_idx,
                                                                       from_layer,
                                                                       to_layer,
                                                                       dx,
                                                                       dy);
                        delays_[physical_tile_type_idx][from_layer][to_layer][dx][dy] = min_delay;
                    }
                }
            }
        }
    }
}

float SimpleDelayModel::delay(const t_physical_tile_loc& from_loc, int /*from_pin*/, const t_physical_tile_loc& to_loc, int /*to_pin*/) const {
    int delta_x = std::abs(from_loc.x - to_loc.x);
    int delta_y = std::abs(from_loc.y - to_loc.y);

    int from_tile_idx = g_vpr_ctx.device().grid.get_physical_type(from_loc)->index;
    return delays_[from_tile_idx][from_loc.layer_num][to_loc.layer_num][delta_x][delta_y];
}

void SimpleDelayModel::read(const std::string& file) {
#ifndef VTR_ENABLE_CAPNPROTO
    (void)file;
    VPR_THROW(VPR_ERROR_PLACE,
              "SimpleDelayModel::read is disabled because VTR_ENABLE_CAPNPROTO=OFF. "
              "Re-compile with CMake option VTR_ENABLE_CAPNPROTO=ON to enable.");
#else
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

    auto toFloat = [](float* out, const VprFloatEntry::Reader& in) -> void {
        *out = in.getValue();
    };

    // ToNdMatrix is a generic function for converting a Matrix capnproto
    // to a vtr::NdMatrix.
    //
    // The user must supply the matrix dimension (5 in this case), the source
    // capnproto type (VprFloatEntry),
    // target C++ type (flat), and a function to convert from the source capnproto
    // type to the target C++ type (ToFloat).
    //
    // The second argument should be of type Matrix<X>::Reader where X is the
    // capnproto element type.
    ToNdMatrix<5, VprFloatEntry, float>(&delays_, model.getDelays(), toFloat);
#endif
}

void SimpleDelayModel::write(const std::string& file) const {
#ifndef VTR_ENABLE_CAPNPROTO
    (void)file;
    VPR_THROW(VPR_ERROR_PLACE,
              "SimpleDelayModel::write is disabled because VTR_ENABLE_CAPNPROTO=OFF. "
              "Re-compile with CMake option VTR_ENABLE_CAPNPROTO=ON to enable.");
#else
    // MallocMessageBuilder object generates capnproto message builder,
    // using malloc for buffer allocation.
    ::capnp::MallocMessageBuilder builder;

    // initRoot<X> returns a X::Builder object that can be used to set the
    // fields in the message.
    auto model = builder.initRoot<VprDeltaDelayModel>();

    auto fromFloat = [](VprFloatEntry::Builder* out, const float& in) -> void {
        out->setValue(in);
    };

    // FromNdMatrix is a generic function for converting a vtr::NdMatrix to a
    // Matrix message.  It is the mirror function of ToNdMatrix described in
    // read above.
    auto delay_values = model.getDelays();
    FromNdMatrix<5, VprFloatEntry, float>(&delay_values, delays_, fromFloat);

    // writeMessageToFile writes message to the specified file.
    writeMessageToFile(file, &builder);
#endif
}
