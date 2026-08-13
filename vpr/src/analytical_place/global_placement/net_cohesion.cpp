/**
 * @file
 * @brief Structural net-cohesion detection (see net_cohesion.h).
 */

#include "net_cohesion.h"

#include <algorithm>
#include <cctype>
#include <cmath>

#include "atom_netlist.h"
#include "flat_placement_density_manager.h"
#include "logic_types.h"
#include "partial_placement.h"
#include "vtr_assert.h"
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

/**
 * @brief Minimum warm-start HPWL, as a fraction of device span, for applying
 *        boundary-net cohesion.
 */
constexpr double kBoundaryNetCohesionMinSeedHpwlFraction = 0.25;

constexpr double kEpsilon = 1e-9;

std::string lower_copy(const std::string& value) {
    std::string lowered = value;
    for (char& c : lowered)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return lowered;
}

} // namespace

bool model_name_is_io_chain(const std::string& model_name) {
    if (model_name == LogicalModels::MODEL_INPUT || model_name == LogicalModels::MODEL_OUTPUT)
        return true;

    std::string lowered = lower_copy(model_name);
    // delay_chain: I/O delay-chain primitives are part of the pad periphery
    // family (io_config -> delay_chain -> ddio/obuf -> pad). Every other member
    // already matches, so a delay-chain atom on a net used to disqualify
    // exactly the periphery nets whose separation produced LU_Network's
    // 13ns bad-basin critical path (5ns hops around the delay chains).
    return lowered.find("io") != std::string::npos
           || lowered.find("pad") != std::string::npos
           || lowered.find("obuf") != std::string::npos
           || lowered.find("oct") != std::string::npos
           || lowered.find("delay_chain") != std::string::npos
           || lowered.find("termination") != std::string::npos;
}

NetCohesion::NetCohesion(const APNetlist& ap_netlist,
                         const AtomNetlist& atom_netlist,
                         const LogicalModels& models,
                         const FlatPlacementDensityManager& density_manager,
                         size_t device_grid_width,
                         size_t device_grid_height,
                         size_t device_grid_num_layers,
                         double boundary_net_weight,
                         double io_chain_net_weight,
                         int log_verbosity)
    : ap_netlist_(ap_netlist)
    , atom_netlist_(atom_netlist)
    , models_(models)
    , density_manager_(density_manager)
    , device_grid_width_(device_grid_width)
    , device_grid_height_(device_grid_height)
    , device_grid_num_layers_(device_grid_num_layers)
    , boundary_net_weight_(boundary_net_weight)
    , io_chain_net_weight_(io_chain_net_weight)
    , log_verbosity_(log_verbosity)
    , boundary_cohesion_nets_(ap_netlist.nets().size(), false)
    , io_chain_cohesion_nets_(ap_netlist.nets().size(), false) {}

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

    PrimitiveVector block_mass = density_manager_.mass_calculator().get_block_mass(blk_id);
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        double mass = block_mass.get_dim_val(dimensions[dim_idx]);
        if (mass != 0. && boundary_confined_dims_[dim_idx])
            return true;
    }
    return false;
}

void NetCohesion::update_boundary_net_flags(const std::vector<PrimitiveVectorDim>& dimensions,
                                            const PartialPlacement& seed) {
    boundary_cohesion_nets_.resize(ap_netlist_.nets().size(), false);
    std::fill(boundary_cohesion_nets_.begin(), boundary_cohesion_nets_.end(), false);
    io_chain_cohesion_nets_.resize(ap_netlist_.nets().size(), false);
    std::fill(io_chain_cohesion_nets_.begin(), io_chain_cohesion_nets_.end(), false);
    num_io_chain_cohesion_nets_ = 0;

    size_t boundary_blocks = 0;
    size_t io_chain_blocks = 0;
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        bool has_boundary_mass = block_has_boundary_mass(blk_id, dimensions);
        if (has_boundary_mass)
            boundary_blocks++;
        if (has_boundary_mass && block_is_io_chain_block_(blk_id))
            io_chain_blocks++;
    }

    size_t boundary_nets = 0;
    size_t io_chain_nets = 0;
    for (APNetId net_id : ap_netlist_.nets()) {
        if (ap_netlist_.net_is_ignored(net_id))
            continue;
        if (ap_netlist_.net_pins(net_id).size() != 2)
            continue;

        bool has_boundary_endpoint = false;
        APBlockId first_blk_id = APBlockId::INVALID();
        APBlockId second_blk_id = APBlockId::INVALID();
        for (APPinId pin_id : ap_netlist_.net_pins(net_id)) {
            APBlockId blk_id = ap_netlist_.pin_block(pin_id);
            if (!first_blk_id.is_valid())
                first_blk_id = blk_id;
            else
                second_blk_id = blk_id;
            has_boundary_endpoint = has_boundary_endpoint || block_has_boundary_mass(blk_id, dimensions);
        }
        if (!has_boundary_endpoint)
            continue;

        // Direct I/O-chain nets are flagged regardless of their seed length.
        // The periphery tearing that motivates this weight happens during the
        // epochs, when partial legalization scatters scarce I/O resources --
        // not in the warm-start seed -- so a seed-length gate misses exactly
        // the nets that need cohesion, and extra weight on an already-short
        // net only keeps it short. The seed-length gate below is retained for
        // the generic boundary-cohesion class. Measured 2026-08-10 (with the
        // delay_chain matcher and weight 8): LU_Network routed CPD 13.1/13.1/
        // 16.9/13.0 -> 7.4/7.7/7.2/7.4 ns across four seeds, stereo_vision
        // CPD/WL also improve, all 73 other circuits bit-identical.
        if (block_has_boundary_mass(first_blk_id, dimensions)
            && block_has_boundary_mass(second_blk_id, dimensions)
            && block_is_io_chain_block_(first_blk_id)
            && block_is_io_chain_block_(second_blk_id)) {
            io_chain_cohesion_nets_[net_id] = true;
            io_chain_nets++;
        }

        double seed_hpwl = std::abs(seed.block_x_locs[first_blk_id] - seed.block_x_locs[second_blk_id])
                           + std::abs(seed.block_y_locs[first_blk_id] - seed.block_y_locs[second_blk_id]);
        double device_span = std::max<double>(device_grid_width_, device_grid_height_);
        if (seed_hpwl < kBoundaryNetCohesionMinSeedHpwlFraction * device_span)
            continue;

        boundary_cohesion_nets_[net_id] = true;
        boundary_nets++;
    }
    num_io_chain_cohesion_nets_ = io_chain_nets;

    if (log_verbosity_ >= 1) {
        VTR_LOG("Nonlinear Nesterov boundary-net cohesion: %zu boundary-mass blocks, %zu long two-pin boundary-related nets, weight=%g, min_seed_hpwl_frac=%g.\n",
                boundary_blocks,
                boundary_nets,
                boundary_net_weight_,
                kBoundaryNetCohesionMinSeedHpwlFraction);
        VTR_LOG("Nonlinear Nesterov I/O-chain cohesion: %zu boundary I/O-chain blocks, %zu long direct I/O-chain nets, weight=%g.\n",
                io_chain_blocks,
                io_chain_nets,
                io_chain_net_weight_);
    }
}

bool NetCohesion::block_is_io_chain_block_(APBlockId blk_id) const {
    bool saw_atom = false;

    for (APPinId pin_id : ap_netlist_.block_pins(blk_id)) {
        AtomPinId atom_pin_id = ap_netlist_.pin_atom_pin(pin_id);
        if (!atom_pin_id.is_valid())
            continue;

        AtomBlockId atom_blk_id = atom_netlist_.pin_block(atom_pin_id);
        if (!atom_blk_id.is_valid())
            continue;

        saw_atom = true;
        LogicalModelId model_id = atom_netlist_.block_model(atom_blk_id);
        if (!model_name_is_io_chain(models_.model_name(model_id)))
            return false;
    }

    return saw_atom;
}
