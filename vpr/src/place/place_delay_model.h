#ifndef PLACE_DELAY_MODEL_H
#define PLACE_DELAY_MODEL_H

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

//Abstract interface to a placement delay model
class PlaceDelayModel {
  public:
    virtual ~PlaceDelayModel() = default;

    // Computes place delay model.
    virtual void compute(
        RouterDelayProfiler& route_profiler,
        const t_placer_opts& placer_opts,
        const t_router_opts& router_opts,
        int longest_length)
        = 0;

    //Returns the delay estimate between the specified block pins
    //
    // Either compute or read methods must be invoked before invoking
    // delay.
    virtual float delay(int from_x, int from_y, int from_pin, int to_x, int to_y, int to_pin) const = 0;

    //Dumps the delay model to an echo file
    virtual void dump_echo(std::string filename) const = 0;

    // Write place delay model to specified file.
    // May be unimplemented, in which case method should throw an exception.
    virtual void write(const std::string& file) const = 0;

    // Read place delay model from specified file.
    // May be unimplemented, in which case method should throw an exception.
    virtual void read(const std::string& file) = 0;
};

//A simple delay model based on the distance (delta) between block locations
class DeltaDelayModel : public PlaceDelayModel {
  public:
    DeltaDelayModel() {}
    DeltaDelayModel(vtr::Matrix<float> delta_delays)
        : delays_(std::move(delta_delays)) {}

    void compute(
        RouterDelayProfiler& router,
        const t_placer_opts& placer_opts,
        const t_router_opts& router_opts,
        int longest_length) override;
    float delay(int from_x, int from_y, int /*from_pin*/, int to_x, int to_y, int /*to_pin*/) const override;
    void dump_echo(std::string filepath) const override;

    void read(const std::string& file) override;
    void write(const std::string& file) const override;
    const vtr::Matrix<float>& delays() const {
        return delays_;
    }

  private:
    vtr::Matrix<float> delays_;
};

class OverrideDelayModel : public PlaceDelayModel {
  public:
    void compute(
        RouterDelayProfiler& route_profiler,
        const t_placer_opts& placer_opts,
        const t_router_opts& router_opts,
        int longest_length) override;
    float delay(int from_x, int from_y, int from_pin, int to_x, int to_y, int to_pin) const override;
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

    void compute_override_delay_model(RouterDelayProfiler& router,
                                      const t_router_opts& router_opts);

    struct t_override {
        short from_type;
        short to_type;
        short from_class;
        short to_class;
        short delta_x;
        short delta_y;

        //A combination of ALWAYS_INLINE attribute and std::lexicographical_compare
        //is required for operator< to be inlined by compiler.
        //Proper inlining of the function reduces place time by around 5%.
        //For more information: https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/1225
        friend ALWAYS_INLINE bool operator<(const t_override& lhs, const t_override& rhs) {
            const short* left = reinterpret_cast<const short*>(&lhs);
            const short* right = reinterpret_cast<const short*>(&rhs);
            constexpr size_t NUM_T_OVERRIDE_MEMBERS = sizeof(t_override) / sizeof(short);
            return std::lexicographical_compare(left, left + NUM_T_OVERRIDE_MEMBERS, right, right + NUM_T_OVERRIDE_MEMBERS);
        }
    };

    vtr::flat_map2<t_override, float> delay_overrides_;

    //operator< treats memory layout of t_override as an array of short
    //this requires all members of t_override are shorts and there is no padding between members of t_override
    static_assert(sizeof(t_override) == sizeof(t_override::from_type) + sizeof(t_override::to_type) + sizeof(t_override::from_class) + sizeof(t_override::to_class) + sizeof(t_override::delta_x) + sizeof(t_override::delta_y), "Expect t_override to have a memory layout equivalent to an array of short (no padding)");
    static_assert(sizeof(t_override::from_type) == sizeof(short), "Expect all t_override data members to be shorts");
    static_assert(sizeof(t_override::to_type) == sizeof(short), "Expect all t_override data members to be shorts");
    static_assert(sizeof(t_override::from_class) == sizeof(short), "Expect all t_override data members to be shorts");
    static_assert(sizeof(t_override::to_class) == sizeof(short), "Expect all t_override data members to be shorts");
    static_assert(sizeof(t_override::delta_x) == sizeof(short), "Expect all t_override data members to be shorts");
    static_assert(sizeof(t_override::delta_y) == sizeof(short), "Expect all t_override data members to be shorts");
};

#endif
