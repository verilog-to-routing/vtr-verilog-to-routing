/**
 * @file place_delay_model.h
 * @brief This file contains all the class and function declarations related to
 *        the placer delay model. For implementations, see place_delay_model.cpp.
 */

#pragma once
#include "vtr_ndmatrix.h"
#include "vtr_flat_map.h"
#include "vpr_types.h"
#include "router_delay_profiling.h"

#ifndef __has_attribute
#    define __has_attribute(x) 0 // Compatibility with non-clang compilers.
#endif

#if defined(COMPILER_GCC) && defined(NDEBUG)
#    define ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif defined(COMPILER_MSVC) && defined(NDEBUG)
#    define ALWAYS_INLINE __forceinline
#elif __has_attribute(always_inline)
#    define ALWAYS_INLINE __attribute__((always_inline)) // clang
#else
#    define ALWAYS_INLINE inline
#endif

///@brief Forward declarations.
class PlaceDelayModel;
class PlacerState;

///@brief Initialize the placer delay model.
std::unique_ptr<PlaceDelayModel> alloc_lookups_and_delay_model(const Netlist<>& net_list,
                                                               t_chan_width_dist chan_width_dist,
                                                               const t_placer_opts& place_opts,
                                                               const t_router_opts& router_opts,
                                                               t_det_routing_arch* det_routing_arch,
                                                               std::vector<t_segment_inf>& segment_inf,
                                                               const std::vector<t_direct_inf>& directs,
                                                               bool is_flat);

///@brief Returns the delay of one point to point connection.
float comp_td_single_connection_delay(const PlaceDelayModel* delay_model,
                                      const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs,
                                      ClusterNetId net_id,
                                      int ipin);

///@brief Recompute all point to point delays, updating `connection_delay` matrix.
void comp_td_connection_delays(const PlaceDelayModel* delay_model,
                               PlacerState& placer_state);

///@brief Abstract interface to a placement delay model.
class PlaceDelayModel {
  public:
    virtual ~PlaceDelayModel() = default;

    ///@brief Computes place delay model.
    virtual void compute(
        RouterDelayProfiler& route_profiler,
        const t_placer_opts& placer_opts,
        const t_router_opts& router_opts,
        int longest_length)
        = 0;

    /**
     * @brief Returns the delay estimate between the specified block pins.
     *
     * Either compute or read methods must be invoked before invoking delay.
     */
    virtual float delay(const t_physical_tile_loc& from_loc, int from_pin, const t_physical_tile_loc& to_loc, int to_pin) const = 0;

    ///@brief Dumps the delay model to an echo file.
    virtual void dump_echo(std::string filename) const = 0;

    /**
     * @brief Write place delay model to specified file.
     *
     * May be unimplemented, in which case method should throw an exception.
     */
    virtual void write(const std::string& file) const = 0;

    /**
     * @brief Read place delay model from specified file.
     *
     * May be unimplemented, in which case method should throw an exception.
     */
    virtual void read(const std::string& file) = 0;
};

///@brief A simple delay model based on the distance (delta) between block locations.
class DeltaDelayModel : public PlaceDelayModel {
  public:
    DeltaDelayModel(float min_cross_layer_delay,
                    bool is_flat)
        : cross_layer_delay_(min_cross_layer_delay)
        , is_flat_(is_flat) {}
    DeltaDelayModel(float min_cross_layer_delay,
                    vtr::NdMatrix<float, 4> delta_delays,
                    bool is_flat)
        : delays_(std::move(delta_delays))
        , cross_layer_delay_(min_cross_layer_delay)
        , is_flat_(is_flat) {}

    void compute(
        RouterDelayProfiler& router,
        const t_placer_opts& placer_opts,
        const t_router_opts& router_opts,
        int longest_length) override;
    float delay(const t_physical_tile_loc& from_loc, int /*from_pin*/, const t_physical_tile_loc& to_loc, int /*to_pin*/) const override;
    void dump_echo(std::string filepath) const override;

    void read(const std::string& file) override;
    void write(const std::string& file) const override;
    const vtr::NdMatrix<float, 4>& delays() const {
        return delays_;
    }

  private:
    vtr::NdMatrix<float, 4> delays_; // [0..num_layers-1][0..max_dx][0..max_dy]
    float cross_layer_delay_;
    /**
     * @brief Indicates whether the router is a two-stage or run-flat
     */
    bool is_flat_;
};

class OverrideDelayModel : public PlaceDelayModel {
  public:
    OverrideDelayModel(float min_cross_layer_delay,
                       bool is_flat)
        : cross_layer_delay_(min_cross_layer_delay)
        , is_flat_(is_flat) {}
    void compute(
        RouterDelayProfiler& route_profiler,
        const t_placer_opts& placer_opts,
        const t_router_opts& router_opts,
        int longest_length) override;
    // returns delay from the specified (x,y) to the specified (x,y) with both endpoints on layer_num and the
    // specified from and to pins
    float delay(const t_physical_tile_loc& from_loc, int from_pin, const t_physical_tile_loc& to_loc, int to_pin) const override;
    void dump_echo(std::string filepath) const override;

    void read(const std::string& file) override;
    void write(const std::string& file) const override;

  public: //Mutators
    void set_base_delay_model(std::unique_ptr<DeltaDelayModel> base_delay_model);
    const DeltaDelayModel* base_delay_model() const;
    float get_delay_override(int from_type, int from_class, int to_type, int to_class, int delta_x, int delta_y) const;
    void set_delay_override(int from_type, int from_class, int to_type, int to_class, int delta_x, int delta_y, float delay);

  private:
    std::unique_ptr<DeltaDelayModel> base_delay_model_;
    /**
     * @brief Minimum delay of cross-layer connections
     */
    float cross_layer_delay_;
    /**
     * @brief Indicates whether the router is a two-stage or run-flat
     */
    bool is_flat_;

    void compute_override_delay_model(RouterDelayProfiler& router,
                                      const t_router_opts& router_opts);

    /**
     * @brief Structure that allows delays to be queried from the delay model.
     *
     * Delay is calculated given the origin physical tile, the origin
     * pin, the destination physical tile, and the destination pin.
     * This structure encapsulates all these information.
     *
     *   @param from_type, to_type
     *              Physical tile index (for easy array access)
     *   @param from_class, to_class
     *              The class that the pins belongs to.
     *   @param to_x, to_y
     *              The horizontal and vertical displacement
     *              between two physical tiles.
     */
    struct t_override {
        short from_type;
        short to_type;
        short from_class;
        short to_class;
        short delta_x;
        short delta_y;

        /**
         * @brief Comparison operator designed for performance.
         *
         * Operator< is important since t_override serves as the key into the
         * map structure delay_overrides_. A default comparison operator would
         * not be inlined by the compiler.
         *
         * A combination of ALWAYS_INLINE attribute and std::lexicographical_compare
         * is required for operator< to be inlined by compiler. Proper inlining of
         * the function reduces place time by around 5%.
         *
         * For more information: https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/1225
         */
        friend ALWAYS_INLINE bool operator<(const t_override& lhs, const t_override& rhs) {
            const short* left = reinterpret_cast<const short*>(&lhs);
            const short* right = reinterpret_cast<const short*>(&rhs);
            constexpr size_t NUM_T_OVERRIDE_MEMBERS = sizeof(t_override) / sizeof(short);
            return std::lexicographical_compare(left, left + NUM_T_OVERRIDE_MEMBERS, right, right + NUM_T_OVERRIDE_MEMBERS);
        }
    };

    /**
     * @brief Map data structure that returns delay values according to
     *        specific delay model queries.
     *
     * Delay model queries are provided by the t_override structure, which
     * encapsulates the information regarding the origin and the destination.
     */
    vtr::flat_map2<t_override, float> delay_overrides_;

    /**
     * operator< treats memory layout of t_override as an array of short.
     * This requires all members of t_override are shorts and there is no
     * padding between members of t_override.
     */
    static_assert(sizeof(t_override) == sizeof(t_override::from_type) + sizeof(t_override::to_type) + sizeof(t_override::from_class) + sizeof(t_override::to_class) + sizeof(t_override::delta_x) + sizeof(t_override::delta_y), "Expect t_override to have a memory layout equivalent to an array of short (no padding)");
    static_assert(sizeof(t_override::from_type) == sizeof(short), "Expect all t_override data members to be shorts");
    static_assert(sizeof(t_override::to_type) == sizeof(short), "Expect all t_override data members to be shorts");
    static_assert(sizeof(t_override::from_class) == sizeof(short), "Expect all t_override data members to be shorts");
    static_assert(sizeof(t_override::to_class) == sizeof(short), "Expect all t_override data members to be shorts");
    static_assert(sizeof(t_override::delta_x) == sizeof(short), "Expect all t_override data members to be shorts");
    static_assert(sizeof(t_override::delta_y) == sizeof(short), "Expect all t_override data members to be shorts");
};

///@brief A simple delay model based on the information stored in router lookahead
///  This is in contrast to other placement delay models that get the cost of getting from one location to another by running the router
class SimpleDelayModel : public PlaceDelayModel {
  public:
    SimpleDelayModel() {}

    void compute(
        RouterDelayProfiler& router,
        const t_placer_opts& placer_opts,
        const t_router_opts& router_opts,
        int longest_length) override;
    float delay(const t_physical_tile_loc& from_loc, int /*from_pin*/, const t_physical_tile_loc& to_loc, int /*to_pin*/) const override;
    void dump_echo(std::string /*filepath*/) const override {}

    void read(const std::string& /*file*/) override {}
    void write(const std::string& /*file*/) const override {}

  private:
    /**
     * @brief The matrix to store the minimum delay between different points on different layers.
     *
     *The matrix used to store delay information is a 5D matrix. This data structure stores the minimum delay for each tile type on each layer to other layers
     *for each dx and dy. We decided to separate the delay for each physical type on each die to accommodate cases where the connectivity of a physical type differs
     *on each layer. Additionally, instead of using d_layer, we distinguish between the destination layer to handle scenarios where connectivity between layers
     *is not uniform. For example, if the number of inter-layer connections between layer 1 and 2 differs from the number of connections between layer 0 and 1.
     *One might argue that this variability could also occur for dx and dy. However, we are operating under the assumption that the FPGA fabric architecture is regular.
     */
    vtr::NdMatrix<float, 5> delays_; // [0..num_physical_type-1][0..num_layers-1][0..num_layers-1][0..max_dx][0..max_dy]
};
