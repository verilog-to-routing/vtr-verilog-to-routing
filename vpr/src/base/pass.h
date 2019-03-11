#ifndef VPR_PASS_H
#define VPR_PASS_H

#include "vpr_context_fwd.h"

/*
 * Somewhere the host application will setup a VPR state context and create
 * passes to be run on the state context.
 *
 * For example:
 *      
 *      int main() {
 *          
 *          VprContext vpr_ctx;
 *
 *          std::vector<std::unique_ptr<Pass>> passes;
 *
 *          //Add various passes
 *          passes.emplace_back(std::make_unique<MyPass>(vpr_ctx);
 *          passes.emplace_back(std::make_unique<OtherPass>(vpr_ctx);
 *          ...
 *             
 *          //Run all passes
 *          for (auto& pass : passes) {
 *              pass->run_pass();
 *          }
 *      }
 */

/*
 * Abstract interface for a pass
 */
class Pass {
    public:
        virtual bool run_pass() = 0;
};

/*
 * Mutable (read/wrtie) pass
 *
 * Mutable passes can modify their context
 */
class MutablePass : public Pass {
    public:
        MutablePass(VprContext& vpr_ctx);

        //All passes can access the full context immutably (read-only)
        const VprContext& ctx() const { return vpr_ctx_; }

        //Deligate to run_on_mutable()
        virtual bool run_pass() final { return run_on_mutable(vpr_ctx_); } //Note final prevents sub-classes from re-defining

        //All MutablePasses get access to the full context mutably (read/write)
        virtual bool run_on_mutable(VprContext& mutable_ctx) = 0;

    private:
        VprContext& vpr_ctx_;
};

/*
 * Immutable (read-only) pass
 *
 * Immutable passes can not modify their input context
 */
class ImmutablePass : public Pass {
    public:
        ImmutablePass(const VprContext& vpr_ctx);

        //All passes can access the full context immutably (read-only)
        const VprContext& ctx() const { return vpr_ctx_; }

        //Deligate to run_on_immutable()
        virtual bool run_pass() final { return run_on_immutable(vpr_ctx_); } //Note final prevents sub-classes from re-defining

        //All ImmutablePasses implement run_on_immutable() which provides immutable (read-only)
        //acess to the entire VPR context
        virtual bool run_on_immutable(const VprContext& immutable_ctx) = 0;

    private:
        const VprContext& vpr_ctx_;
};

/*
 * Restricted passes allowing the mutation of only one specific part of the overall context
 */
class AtomPass : public MutablePass {
    public:
        //Deligate to run_on_atom()
        virtual bool run_on_mutable(VprContext& ctx) final { return run_on_atom(ctx.mutable_atom()); } //Note final prevents sub-classes from re-defining

        //All AtomPass derived passes only get mutable access to the AtomContext.
        //Note that they can still get immutable access to the full context via ctx().
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
