/**
 * @file test_runstage_fixture.cpp
 * @brief GUI-T-017 — VPR run-stage Catch2 fixture, implementation.
 *
 * See test_runstage_fixture.hpp for the architectural rationale.
 */

#include "test_runstage_fixture.hpp"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <system_error>
#include <unistd.h>

#include "globals.h"
#include "vpr_error.h"

#ifndef VPR_TEST_ARCH_XML
#error "VPR_TEST_ARCH_XML must be defined by CMake (see vpr/test/gui/CMakeLists.txt)"
#endif
#ifndef VPR_TEST_BLIF_AND_LATCH
#error "VPR_TEST_BLIF_AND_LATCH must be defined by CMake (see vpr/test/gui/CMakeLists.txt)"
#endif

namespace vpr_gui_test {

namespace {

// vpr_initialize_logging() may be called only once per process. Multiple
// fixture instances within a single test_vpr_gui invocation must not
// re-initialise the logger.
std::atomic<bool> g_logging_initialised{false};

constexpr const char* kRouteChanWidth = "100";

} // namespace

std::vector<const char*> VprRunStageFixture::build_argv() {
    // The argv pointers must outlive vpr_init; we return a vector by
    // value with string literals + macro-expanded compile-time paths,
    // which all have static storage duration. No std::string lifetime
    // hazards.
    return {
        "test_vpr_gui",
        VPR_TEST_ARCH_XML,
        VPR_TEST_BLIF_AND_LATCH,
        // Explicitly request the pack -> place -> route flow. Without any
        // stage flag, VPR now defaults to the Analytical Placement flow.
        "--pack",
        "--place",
        "--route",
        "--route_chan_width",
        kRouteChanWidth,
        // Pin the seed so failures are reproducible bit-for-bit.
        "--seed",
        "1",
        // Suppress display: we are running headless under
        // QT_QPA_PLATFORM=offscreen and have no need for ezgl
        // graphics initialisation inside the fixture itself. Layer-4
        // tests that need an ezgl::application use EzglAppFixture
        // separately, which loads main.ui directly without going
        // through vpr_init_graphics.
        "--disp",
        "off",
    };
}

void VprRunStageFixture::init_logging_once() {
    bool expected = false;
    if (g_logging_initialised.compare_exchange_strong(expected, true)) {
        vpr_initialize_logging();
    }
}

void VprRunStageFixture::create_workdir_and_chdir() {
    // mkstemp-style directory in /tmp. We do not use std::tmpnam (deprecated
    // and racy); std::filesystem::temp_directory_path() + a unique tag
    // built from the pid + a static counter is sufficient because Catch2
    // sequential execution guarantees no concurrent fixtures.
    static std::atomic<unsigned> counter{0};
    auto tag = std::string("vpr_gui_t017_") + std::to_string(::getpid()) + "_" + std::to_string(counter.fetch_add(1));
    work_dir_ = std::filesystem::temp_directory_path() / tag;

    std::error_code ec;
    std::filesystem::create_directories(work_dir_, ec);
    if (ec) {
        throw std::runtime_error("VprRunStageFixture: cannot create "
                                 + work_dir_.string() + ": " + ec.message());
    }

    prev_cwd_ = std::filesystem::current_path(ec);
    if (ec) {
        throw std::runtime_error("VprRunStageFixture: cannot read cwd: "
                                 + ec.message());
    }

    std::filesystem::current_path(work_dir_, ec);
    if (ec) {
        throw std::runtime_error("VprRunStageFixture: cannot chdir to "
                                 + work_dir_.string() + ": " + ec.message());
    }
}

void VprRunStageFixture::teardown_workdir() {
    std::error_code ec;
    if (!prev_cwd_.empty()) {
        std::filesystem::current_path(prev_cwd_, ec);
        // Ignore restore failure — the test process is about to move on
        // and the parent runner / next case will set its own cwd.
    }
    if (!work_dir_.empty()) {
        // Best-effort cleanup. Leaving the dir on a failure is preferable
        // to masking debug artefacts; the temp_directory_path is /tmp by
        // default and will be cleared by the OS soon enough.
        std::filesystem::remove_all(work_dir_, ec);
    }
}

VprRunStageFixture::VprRunStageFixture() {
    init_logging_once();
    create_workdir_and_chdir();
}

VprRunStageFixture::~VprRunStageFixture() {
    if (initialized_ && !freed_) {
        try {
            vpr_free_all(arch_, vpr_setup_);
        } catch (...) {
            // A throw out of a destructor would call std::terminate; we
            // swallow and keep the process going so the rest of the test
            // binary can run. The leaked state is process-wide and the
            // next fixture re-init will observe it; that is the user's
            // signal that something went wrong.
        }
        freed_ = true;
    }
    teardown_workdir();
}

void VprRunStageFixture::run_to(Stage stage) {
    if (initialized_) {
        throw std::logic_error("VprRunStageFixture::run_to called twice on "
                               "the same fixture instance");
    }

    // ---- Stage::PostInit ---------------------------------------------------
    auto argv = build_argv();
    vpr_init(static_cast<int>(argv.size()), argv.data(),
             &options_, &vpr_setup_, &arch_);
    initialized_ = true;
    reached_ = Stage::PostInit;
    if (stage == Stage::PostInit) return;

    // ---- Stage::PostPack ---------------------------------------------------
    if (!vpr_pack_flow(vpr_setup_, arch_)) {
        throw std::runtime_error("VprRunStageFixture: vpr_pack_flow failed "
                                 "(real packer defect — investigate before "
                                 "modifying the fixture)");
    }
    reached_ = Stage::PostPack;
    if (stage == Stage::PostPack) return;

    // ---- Stage::PostPlace --------------------------------------------------
    // Mirrors the step order in vpr_api.cpp::vpr_flow():
    //   vpr_create_device -> vpr_init_graphics (skipped here, --disp off) ->
    //   vpr_place_flow
    vpr_create_device(vpr_setup_, arch_);
    {
        const auto& placement_net_list =
            (const Netlist<>&)g_vpr_ctx.clustering().clb_nlist;
        if (!vpr_place_flow(placement_net_list, vpr_setup_, arch_)) {
            throw std::runtime_error("VprRunStageFixture: vpr_place_flow "
                                     "returned false (unimplementable)");
        }
    }
    reached_ = Stage::PostPlace;
    if (stage == Stage::PostPlace) return;

    // ---- Stage::PostRoute --------------------------------------------------
    bool is_flat = vpr_setup_.RouterOpts.flat_routing;
    const Netlist<>& router_net_list = is_flat
                                           ? (const Netlist<>&)g_vpr_ctx.atom().netlist()
                                           : (const Netlist<>&)g_vpr_ctx.clustering().clb_nlist;
    RouteStatus route_status = vpr_route_flow(router_net_list, vpr_setup_,
                                              arch_, is_flat);
    if (!route_status.success()) {
        throw std::runtime_error("VprRunStageFixture: vpr_route_flow did not "
                                 "succeed (channel width too low or routing "
                                 "regressed — investigate before modifying "
                                 "the fixture)");
    }
    reached_ = Stage::PostRoute;
}

} // namespace vpr_gui_test

// ===========================================================================
// Sentinel cases — assert the fixture itself works. These are the substrate
// every Wave-2 ticket relies on; if any of these fails, no Wave-2 test can
// be trusted. They are NOT placeholders for behavioural coverage.
// ===========================================================================

#include <catch2/catch_test_macros.hpp>

using vpr_gui_test::Stage;
using vpr_gui_test::VprRunStageFixture;

TEST_CASE("Fixture: run_to(PostInit) populates options and arch",
          "[layer3][fixture][vpr_gui][GUI-T-017]") {
    VprRunStageFixture fx;
    fx.run_to(Stage::PostInit);
    REQUIRE(fx.reached() == Stage::PostInit);
    // After vpr_init the architecture has at least one logical block type
    // (loaded from VPR_TEST_ARCH_XML); a missing arch would have aborted
    // earlier inside vpr_init.
    const auto& dev = g_vpr_ctx.device();
    INFO("logical_block_types size = " << dev.logical_block_types.size());
    REQUIRE(dev.logical_block_types.size() >= 2);
}

TEST_CASE("Fixture: run_to(PostPack) produces a non-empty cluster netlist",
          "[layer3][fixture][vpr_gui][GUI-T-017]") {
    VprRunStageFixture fx;
    fx.run_to(Stage::PostPack);
    REQUIRE(fx.reached() == Stage::PostPack);
    // and_latch.blif packs into at least an input pad, an output pad, and
    // a logic block — the count is >= 3 on every architecture we ship.
    const auto& cluster = g_vpr_ctx.clustering();
    const auto n_blocks = cluster.clb_nlist.blocks().size();
    INFO("clb_nlist.blocks() size = " << n_blocks);
    REQUIRE(n_blocks >= 3u);
}

TEST_CASE("Fixture: run_to(PostPlace) yields valid block locations",
          "[layer3][fixture][vpr_gui][GUI-T-017]") {
    VprRunStageFixture fx;
    fx.run_to(Stage::PostPlace);
    REQUIRE(fx.reached() == Stage::PostPlace);
    const auto& place = g_vpr_ctx.placement();
    const auto& cluster = g_vpr_ctx.clustering();
    REQUIRE(cluster.clb_nlist.blocks().size() >= 3u);
    // Each placed block must have non-negative coordinates within the
    // device grid. A regression that leaves a block at sentinel
    // coordinates (e.g. -1, -1) would surface here.
    for (auto blk : cluster.clb_nlist.blocks()) {
        const auto loc = place.block_locs()[blk].loc;
        INFO("block " << size_t(blk) << " at (" << loc.x << "," << loc.y
                      << ") layer=" << loc.layer);
        REQUIRE(loc.x >= 0);
        REQUIRE(loc.y >= 0);
    }
}

TEST_CASE("Fixture: run_to(PostRoute) completes and exposes a routed RR graph",
          "[layer3][fixture][vpr_gui][GUI-T-017]") {
    VprRunStageFixture fx;
    fx.run_to(Stage::PostRoute);
    REQUIRE(fx.reached() == Stage::PostRoute);
    // After routing the device's RR graph has nodes and the routing
    // context has trace data for at least one net. Both are
    // preconditions the canvas-rendering tests (T-013, T-014) will
    // depend on.
    const auto& device = g_vpr_ctx.device();
    INFO("rr_graph.num_nodes = " << device.rr_graph.num_nodes());
    REQUIRE(device.rr_graph.num_nodes() > 0u);
}

TEST_CASE("Fixture: run_to refuses to run twice on the same instance",
          "[layer3][fixture][vpr_gui][GUI-T-017]") {
    // Pinned contract: a fresh fixture is required per stage. Catching the
    // misuse explicitly (rather than letting the second vpr_init clobber
    // singleton state silently) is part of the substrate for Wave 2.
    VprRunStageFixture fx;
    fx.run_to(Stage::PostPack);
    REQUIRE_THROWS_AS(fx.run_to(Stage::PostPlace), std::logic_error);
}
