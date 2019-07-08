#ifndef PLACE_DELAY_MODEL_H
#define PLACE_DELAY_MODEL_H

#include "vtr_ndmatrix.h"
#include "vtr_flat_map.h"
#include "vpr_types.h"
#include "router_delay_profiling.h"

//Abstract interface to a placement delay model
class PlaceDelayModel {
  public:
    virtual ~PlaceDelayModel() = default;

    // Computes place delay model.
    virtual void compute(
        const RouterDelayProfile& router,
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

    // Read place delay model to specified file.
    // May be unimplemented, in which case method should throw an exception.
    virtual void read(const std::string& file) = 0;
};

//A simple delay model based on the distance (delta) between block locations
class DeltaDelayModel : public PlaceDelayModel {
  public:
    DeltaDelayModel(){};
    DeltaDelayModel(vtr::Matrix<float> delta_delays, t_router_opts router_opts)
        : delays_(std::move(delta_delays))
        , router_opts_(router_opts) {}

    void compute(
        const RouterDelayProfile& router,
        const t_placer_opts& placer_opts,
        const t_router_opts& router_opts,
        int longest_length) override;
    float delay(int from_x, int from_y, int /*from_pin*/, int to_x, int to_y, int /*to_pin*/) const override;
    void dump_echo(std::string filepath) const override;

    void read(const std::string& file) override {
        (void)file;
        VPR_THROW(VPR_ERROR_ROUTE, "DeltaDelayModel::read unimplemented");
    }
    void write(const std::string& file) const override {
        (void)file;
        VPR_THROW(VPR_ERROR_ROUTE, "DeltaDelayModel::write unimplemented");
    }

  private:
    vtr::Matrix<float> delays_;
    t_router_opts router_opts_;
};

class OverrideDelayModel : public PlaceDelayModel {
  public:
    void compute(
        const RouterDelayProfile& router,
        const t_placer_opts& placer_opts,
        const t_router_opts& router_opts,
        int longest_length) override;
    float delay(int from_x, int from_y, int from_pin, int to_x, int to_y, int to_pin) const override;
    void dump_echo(std::string filepath) const override;

    void read(const std::string& file) override {
        (void)file;
        VPR_THROW(VPR_ERROR_ROUTE, "OverrideDelayModel::read unimplemented");
    }
    void write(const std::string& file) const override {
        (void)file;
        VPR_THROW(VPR_ERROR_ROUTE, "OverrideDelayModel::write unimplemented");
    }

  public: //Mutators
  private:
    std::unique_ptr<PlaceDelayModel> base_delay_model_;

    void set_delay_override(int from_type, int from_class, int to_type, int to_class, int delta_x, int delta_y, float delay);
    void compute_override_delay_model(const RouterDelayProfile& router);

    struct t_override {
        short from_type;
        short to_type;
        short from_class;
        short to_class;
        short delta_x;
        short delta_y;

        friend bool operator<(const t_override& lhs, const t_override& rhs) {
            return std::tie(lhs.from_type, lhs.to_type, lhs.from_class, lhs.to_class, lhs.delta_x, lhs.delta_y)
                   < std::tie(rhs.from_type, rhs.to_type, rhs.from_class, rhs.to_class, rhs.delta_x, rhs.delta_y);
        }
    };

    vtr::flat_map2<t_override, float> delay_overrides_;
    t_router_opts router_opts_;
};

#endif
