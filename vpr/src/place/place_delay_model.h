#ifndef PLACE_DELAY_MODEL_H
#define PLACE_DELAY_MODEL_H

#include "vtr_ndmatrix.h"
#include "vtr_flat_map.h"
#include "vpr_types.h"
#include "router_lookahead_map.h"

//Abstract interface to a placement delay model
class PlaceDelayModel {
  public:
    virtual ~PlaceDelayModel() = default;

    //Returns the delay estimate between the specified block pins
    virtual float delay(int from_x, int from_y, int from_pin, int to_x, int to_y, int to_pin) const = 0;

    //Dumps the delay model to an echo file
    virtual void dump_echo(std::string filename) const = 0;
};

//A simple delay model based on the distance (delta) between block locations
class DeltaDelayModel : public PlaceDelayModel {
  public:
    DeltaDelayModel(vtr::Matrix<float> delta_delays, t_router_opts router_opts)
        : delays_(std::move(delta_delays))
        , router_opts_(router_opts) {}

    float delay(int from_x, int from_y, int /*from_pin*/, int to_x, int to_y, int /*to_pin*/) const override;
    void dump_echo(std::string filepath) const override;

  private:
    vtr::Matrix<float> delays_;
    t_router_opts router_opts_;
};

class OverrideDelayModel : public PlaceDelayModel {
  public:
    OverrideDelayModel(std::unique_ptr<PlaceDelayModel> base_delay_model, t_router_opts router_opts)
        : base_delay_model_(std::move(base_delay_model))
        , router_opts_(router_opts) {}

    float delay(int from_x, int from_y, int from_pin, int to_x, int to_y, int to_pin) const override;
    void dump_echo(std::string filepath) const override;

  public: //Mutators
    void set_delay_override(int from_type, int from_class, int to_type, int to_class, int delta_x, int delta_y, float delay);

  private:
    std::unique_ptr<PlaceDelayModel> base_delay_model_;

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
