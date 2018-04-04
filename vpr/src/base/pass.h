#ifndef VPR_PASS_H
#define VPR_PASS_H

#include "vpr_context_fwd.h"

class Pass {
    public:
        virtual bool run_pass() = 0;
};

/*
 * Mutable (read/wrtie) pass
 */
class MutablePass : public Pass {
    public:
        MutablePass(VprContext& vpr_ctx);

        const VprContext& ctx() const { return vpr_ctx_; }

        virtual bool run_pass() final { return run_on_mutable(vpr_ctx_); }

        virtual bool run_on_mutable(VprContext& mutable_ctx) = 0;

    private:
        VprContext& vpr_ctx_;
};

/*
 * Immutable (read-only) pass
 */
class ImmutablePass : public Pass {
    public:
        ImmutablePass(const VprContext& vpr_ctx);

        const VprContext& ctx() const { return vpr_ctx_; }

        virtual bool run_pass() final { return run_on_immutable(vpr_ctx_); }

        virtual bool run_on_immutable(const VprContext& immutable_ctx) = 0;

    private:
        const VprContext& vpr_ctx_;
};

/*
 * Restricted passes allowing the mutation of only one specific part of the overall context
 */
class AtomPass : public MutablePass {
    public:
        virtual bool run_on_mutable(VprContext& ctx) final { return run_on_atom(ctx.mutable_atom()); }

        virtual bool run_on_atom(AtomContext& atom_ctx) = 0;
};

class DevicePass : public MutablePass {
    public:
        virtual bool run_on_mutable(VprContext& ctx) final { return run_on_device(ctx.mutable_device()); }

        virtual bool run_on_device(DeviceContext& device_ctx) = 0;
};

class TimingPass : public MutablePass {
    public:
        virtual bool run_on_mutable(VprContext& ctx) final { return run_on_device(ctx.mutable_timing()); }

        virtual bool run_on_timing(TimingContext& timing_ctx) = 0;
};

class PowerPass : public MutablePass {
    public:
        virtual bool run_on_mutable(VprContext& ctx) final { return run_on_device(ctx.mutable_timing()); }

        virtual bool run_on_power(PowerContext& power_ctx) = 0;
};

class ClusterPass : public MutablePass {
    public:
        virtual bool run_on_mutable(VprContext& ctx) final { return run_on_device(ctx.mutable_clustering()); }

        virtual bool run_on_clustering(ClusteringContext& cluster_ctx) = 0;
};

class PlacePass : public MutablePass {
    public:
        virtual bool run_on_mutable(VprContext& ctx) final { return run_on_device(ctx.mutable_placement()); }

        virtual bool run_on_placement(PlacementContext& place_ctx) = 0;
};

class RoutePass : public MutablePass {
    public:
        virtual bool run_on_mutable(VprContext& ctx) final { return run_on_routing(ctx.mutable_routing()); }

        virtual bool run_on_routing(RoutingContext& route_ctx) = 0;
};

#endif
