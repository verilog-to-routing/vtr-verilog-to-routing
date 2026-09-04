/**
 * @file
 * @brief Structural net-cohesion detection (see net_cohesion.h).
 */

#include "net_cohesion.h"

#include <algorithm>

#include "flat_placement_density_manager.h"
#include "vtr_log.h"

namespace {

/**
 * @brief Device-edge band used to identify resources confined to the boundary.
 *
 * Several architectures leave the true perimeter empty and place I/O-capable
 * tiles one tile in from the edge, so use a two-tile band rather than only x/y
 * equals 0 or max.
 */
constexpr size_t kBoundaryConfinedBandTiles = 2;

/**
 * @brief Fraction of a resource dimension's capacity that must lie in the edge
 *        band before it is treated as boundary-confined.
 */
constexpr double kBoundaryConfinedCapacityFraction = 0.95;

constexpr double kEpsilon = 1e-9;

} // namespace

NetCohesion::NetCohesion(const APNetlist& ap_netlist,
                         const FlatPlacementDensityManager& density_manager,
                         size_t device_grid_width,
                         size_t device_grid_height,
                         size_t device_grid_num_layers,
                         int log_verbosity)
    : ap_netlist_(ap_netlist)
    , density_manager_(density_manager)
    , device_grid_width_(device_grid_width)
    , device_grid_height_(device_grid_height)
    , device_grid_num_layers_(device_grid_num_layers)
    , log_verbosity_(log_verbosity)
    , periphery_pair_nets_(ap_netlist.nets().size(), false) {}

void NetCohesion::identify_boundary_confined_dims(const std::vector<PrimitiveVectorDim>& dimensions) {
    std::vector<bool> boundary_confined(dimensions.size(), false);
    if (dimensions.empty()) {
        boundary_confined_dims_ = std::move(boundary_confined);
        return;
    }

    const FlatPlacementBins& bins = density_manager_.flat_placement_bins();
    size_t width = device_grid_width_;
    size_t height = device_grid_height_;
    size_t num_layers = device_grid_num_layers_;

    // On a device small enough that the edge band covers nearly the whole grid,
    // the capacity test cannot separate a periphery-confined resource from an
    // evenly distributed one -- every resource would look boundary-confined.
    // Detect that from the grid geometry and classify nothing rather than
    // flagging everything.
    size_t interior_width = width > 2 * kBoundaryConfinedBandTiles ? width - 2 * kBoundaryConfinedBandTiles : 0;
    size_t interior_height = height > 2 * kBoundaryConfinedBandTiles ? height - 2 * kBoundaryConfinedBandTiles : 0;
    double interior_fraction = width * height > 0
                                   ? static_cast<double>(interior_width * interior_height)
                                         / static_cast<double>(width * height)
                                   : 0.;
    if (interior_fraction <= 1. - kBoundaryConfinedCapacityFraction) {
        if (log_verbosity_ >= 1) {
            VTR_LOG("Nonlinear Nesterov boundary-confined resource dims: 0 / %zu (device %zux%zu too small to discriminate).\n",
                    dimensions.size(),
                    width,
                    height);
        }
        boundary_confined_dims_ = std::move(boundary_confined);
        return;
    }

    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        double target_total = 0.;
        double boundary_target_total = 0.;
        for (size_t layer = 0; layer < num_layers; layer++) {
            for (size_t x = 0; x < width; x++) {
                for (size_t y = 0; y < height; y++) {
                    FlatPlacementBinId bin_id = density_manager_.get_bin(x, y, layer);
                    const vtr::Rect<double>& region = bins.bin_region(bin_id);
                    double bin_area = std::max(1.0, region.width() * region.height());
                    double target_density = density_manager_.get_bin_target_density(bin_id);
                    double target = density_manager_.get_bin_capacity(bin_id).get_dim_val(dimensions[dim_idx])
                                    * target_density / bin_area;
                    target_total += target;
                    bool in_boundary_band = x < kBoundaryConfinedBandTiles
                                            || y < kBoundaryConfinedBandTiles
                                            || x + kBoundaryConfinedBandTiles >= width
                                            || y + kBoundaryConfinedBandTiles >= height;
                    if (in_boundary_band)
                        boundary_target_total += target;
                }
            }
        }
        boundary_confined[dim_idx] = target_total > kEpsilon
                                     && boundary_target_total >= kBoundaryConfinedCapacityFraction * target_total;
    }

    if (log_verbosity_ >= 1) {
        size_t num_boundary_dims = 0;
        for (bool is_boundary : boundary_confined) {
            if (is_boundary)
                num_boundary_dims++;
        }
        VTR_LOG("Nonlinear Nesterov boundary-confined resource dims: %zu / %zu (edge band=%zu, threshold=%g).\n",
                num_boundary_dims,
                dimensions.size(),
                kBoundaryConfinedBandTiles,
                kBoundaryConfinedCapacityFraction);
    }

    boundary_confined_dims_ = std::move(boundary_confined);
}

bool NetCohesion::block_has_boundary_mass(APBlockId blk_id,
                                          const std::vector<PrimitiveVectorDim>& dimensions) const {
    if (boundary_confined_dims_.size() != dimensions.size())
        return false;

    const PrimitiveVector& block_mass = density_manager_.mass_calculator().get_block_mass(blk_id);
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        double mass = block_mass.get_dim_val(dimensions[dim_idx]);
        if (mass != 0. && boundary_confined_dims_[dim_idx])
            return true;
    }
    return false;
}

void NetCohesion::update_periphery_pair_nets(const std::vector<PrimitiveVectorDim>& dimensions) {
    periphery_pair_nets_.resize(ap_netlist_.nets().size(), false);
    std::fill(periphery_pair_nets_.begin(), periphery_pair_nets_.end(), false);
    num_periphery_pair_nets_ = 0;

    // Boundary mass is a property of the block, not of the net, so evaluate it
    // once per block rather than once per pin occurrence.
    vtr::vector<APBlockId, bool> has_boundary_mass(ap_netlist_.blocks().size(), false);
    size_t boundary_blocks = 0;
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        has_boundary_mass[blk_id] = block_has_boundary_mass(blk_id, dimensions);
        if (has_boundary_mass[blk_id])
            boundary_blocks++;
    }

    for (APNetId net_id : ap_netlist_.nets()) {
        if (ap_netlist_.net_is_ignored(net_id))
            continue;
        if (ap_netlist_.net_pins(net_id).size() != 2)
            continue;

        APBlockId first_blk_id = ap_netlist_.pin_block(*ap_netlist_.net_pins(net_id).begin());
        APBlockId second_blk_id = ap_netlist_.pin_block(*(ap_netlist_.net_pins(net_id).begin() + 1));
        if (!has_boundary_mass[first_blk_id] || !has_boundary_mass[second_blk_id])
            continue;

        // Flagged regardless of how long the net is in the warm-start seed. The
        // periphery tearing this weight targets happens during the epochs, when
        // partial legalization scatters scarce periphery resources, so a
        // seed-length gate misses exactly the nets that need cohesion.
        periphery_pair_nets_[net_id] = true;
        num_periphery_pair_nets_++;
    }

    if (log_verbosity_ >= 1) {
        VTR_LOG("Nonlinear Nesterov periphery-pair cohesion: %zu boundary-mass blocks, %zu two-pin periphery nets.\n",
                boundary_blocks,
                num_periphery_pair_nets_);
    }
}
