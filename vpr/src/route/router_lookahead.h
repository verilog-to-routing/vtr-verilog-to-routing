#ifndef VPR_ROUTER_LOOKAHEAD_H
#define VPR_ROUTER_LOOKAHEAD_H
#include <memory>
#include "vpr_types.h"
#include "vpr_error.h"

struct t_conn_cost_params; //Forward declaration

class RouterLookahead {
  public:
    // Get expected cost from node to target_node.
    //
    // Either compute or read methods must be invoked before invoking
    // get_expected_cost.
    virtual float get_expected_cost(int node, int target_node, const t_conn_cost_params& params, float R_upstream) const = 0;

    // Compute router lookahead (if needed).
    virtual void compute(const std::vector<t_segment_inf>& segment_inf, const std::string& lookahead_search_locations) = 0;

    // Read router lookahead data (if any) from specified file.
    // May be unimplemented, in which case method should throw an exception.
    virtual void read(const std::string& file) = 0;

    // Write router lookahead data (if any) to specified file.
    // May be unimplemented, in which case method should throw an exception.
    virtual void write(const std::string& file) const = 0;

    virtual ~RouterLookahead() {}
};

// Force creation of lookahead object.
//
// This may involve recomputing the lookahead, so only use if lookahead cache
// cannot be used.
std::unique_ptr<RouterLookahead> make_router_lookahead(
    e_router_lookahead router_lookahead_type,
    std::string write_lookahead,
    std::string read_lookahead,
    const std::vector<t_segment_inf>& segment_inf,
    const std::string& lookahead_search_locations);

// Clear router lookahead cache (e.g. when changing or free rrgraph).
void invalidate_router_lookahead_cache();

// Returns lookahead for given rr graph.
//
// Object is cached in RouterContext, but access to cached object should
// performed via this function.
const RouterLookahead* get_cached_router_lookahead(
    e_router_lookahead router_lookahead_type,
    std::string write_lookahead,
    std::string read_lookahead,
    const std::vector<t_segment_inf>& segment_inf,
    const std::string& lookahead_search_locations);

class ClassicLookahead : public RouterLookahead {
  public:
    float get_expected_cost(int node, int target_node, const t_conn_cost_params& params, float R_upstream) const override;
    void compute(const std::vector<t_segment_inf>& /*segment_inf*/,
                 const std::string& /*lookahead_search_locations*/) override {
    }

    void read(const std::string& /*file*/) override {
        VPR_THROW(VPR_ERROR_ROUTE, "ClassicLookahead::read unimplemented");
    }
    void write(const std::string& /*file*/) const override {
        VPR_THROW(VPR_ERROR_ROUTE, "ClassicLookahead::write unimplemented");
    }

  private:
    float classic_wire_lookahead_cost(int node, int target_node, float criticality, float R_upstream) const;
};

class MapLookahead : public RouterLookahead {
  protected:
    float get_expected_cost(int node, int target_node, const t_conn_cost_params& params, float R_upstream) const override;
    void compute(const std::vector<t_segment_inf>& segment_inf,
                 const std::string& /*lookahead_search_locations*/) override;
    void read(const std::string& /*file*/) override {
        VPR_THROW(VPR_ERROR_ROUTE, "MapLookahead::read unimplemented");
    }
    void write(const std::string& /*file*/) const override {
        VPR_THROW(VPR_ERROR_ROUTE, "MapLookahead::write unimplemented");
    }
};

class NoOpLookahead : public RouterLookahead {
  protected:
    float get_expected_cost(int node, int target_node, const t_conn_cost_params& params, float R_upstream) const override;
    void compute(const std::vector<t_segment_inf>& /*segment_inf*/,
                 const std::string& /*lookahead_search_locations*/) override {
    }
    void read(const std::string& /*file*/) override {
        VPR_THROW(VPR_ERROR_ROUTE, "Read not supported for NoOpLookahead");
    }
    void write(const std::string& /*file*/) const override {
        VPR_THROW(VPR_ERROR_ROUTE, "Write not supported for NoOpLookahead");
    }
};

#endif
