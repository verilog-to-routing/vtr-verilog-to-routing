
#include "delta_delay_model.h"

#include "compute_delta_delays_utils.h"

#ifdef VTR_ENABLE_CAPNPROTO
#    include "capnp/serialize.h"
#    include "place_delay_model.capnp.h"
#    include "ndmatrix_serdes.h"
#    include "mmap_file.h"
#    include "serdes_utils.h"
#endif  // VTR_ENABLE_CAPNPROTO

void DeltaDelayModel::compute(RouterDelayProfiler& route_profiler,
                              const t_placer_opts& placer_opts,
                              const t_router_opts& router_opts,
                              int longest_length) {
    delays_ = compute_delta_delay_model(route_profiler,
                                        placer_opts,
                                        router_opts,
                                        /*measure_directconnect=*/true,
                                        longest_length,
                                        is_flat_);
}

float DeltaDelayModel::delay(const t_physical_tile_loc& from_loc, int /*from_pin*/,
                             const t_physical_tile_loc& to_loc, int /*to_pin*/) const {
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

void DeltaDelayModel::read(const std::string& file) {
#ifndef VTR_ENABLE_CAPNPROTO
    (void)file;
    VPR_THROW(VPR_ERROR_PLACE,
              "OverrideDelayModel::read is disabled because VTR_ENABLE_CAPNPROTO=OFF. "
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
    // The user must supply the matrix dimension (2 in this case), the source
    // capnproto type (VprFloatEntry),
    // target C++ type (flat), and a function to convert from the source capnproto
    // type to the target C++ type (ToFloat).
    //
    // The second argument should be of type Matrix<X>::Reader where X is the
    // capnproto element type.
    ToNdMatrix<4, VprFloatEntry, float>(&delays_, model.getDelays(), toFloat);
#endif
}

void DeltaDelayModel::write(const std::string& file) const {
#ifndef VTR_ENABLE_CAPNPROTO
    (void)file;
    VPR_THROW(VPR_ERROR_PLACE,
              "DeltaDelayModel::write is disabled because VTR_ENABLE_CAPNPROTO=OFF. "
              "Re-compile with CMake option VTR_ENABLE_CAPNPROTO=ON to enable.");
#else

    // MallocMessageBuilder object is the generate capnproto message builder,
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
    FromNdMatrix<4, VprFloatEntry, float>(&delay_values, delays_, fromFloat);

    // writeMessageToFile writes message to the specified file.
    writeMessageToFile(file, &builder);
#endif
}
