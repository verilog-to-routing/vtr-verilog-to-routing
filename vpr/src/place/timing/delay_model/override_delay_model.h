
#pragma once

#include "place_delay_model.h"
#include "delta_delay_model.h"

class OverrideDelayModel : public PlaceDelayModel {
  public:
    OverrideDelayModel(float min_cross_layer_delay,
                       bool is_flat)
        : cross_layer_delay_(min_cross_layer_delay)
        , is_flat_(is_flat) {}

    void compute(RouterDelayProfiler& route_profiler,
                 const t_placer_opts& placer_opts,
                 const t_router_opts& router_opts,
                 int longest_length) override;

    /**
     * @brief returns delay from the specified (x,y) to the specified (x,y) with both endpoints on layer_num and the
     * specified from and to pins
     */
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
    /// Minimum delay of cross-layer connections
    float cross_layer_delay_;

    /// Indicates whether the router is a two-stage or run-flat
    bool is_flat_;

    void compute_override_delay_model_(RouterDelayProfiler& router,
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