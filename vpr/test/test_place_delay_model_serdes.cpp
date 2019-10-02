#include "catch.hpp"

#include "place_delay_model.h"

namespace {

#ifdef VTR_ENABLE_CAPNPROTO
static constexpr const char kDeltaDelayBin[] = "test_delta_delay.bin";
static constexpr const char kOverrideDelayBin[] = "test_override_delay.bin";

TEST_CASE("round_trip_delta_delay_model", "[vpr]") {
    constexpr size_t kDimX = 10;
    constexpr size_t kDimY = 10;
    vtr::Matrix<float> delays;
    delays.resize({kDimX, kDimY});

    for (size_t x = 0; x < kDimX; ++x) {
        for (size_t y = 0; y < kDimY; ++y) {
            delays[x][y] = (x + 1) * (y + 1);
        }
    }
    DeltaDelayModel model(std::move(delays));
    const auto& delays1 = model.delays();

    model.write(kDeltaDelayBin);

    DeltaDelayModel model2;
    model2.read(kDeltaDelayBin);

    const auto& delays2 = model2.delays();

    REQUIRE(delays1.size() == delays2.size());
    REQUIRE(delays1.ndims() == delays2.ndims());
    for (size_t dim = 0; dim < delays1.ndims(); ++dim) {
        REQUIRE(delays1.dim_size(dim) == delays2.dim_size(dim));
    }

    for (size_t x = 0; x < kDimX; ++x) {
        for (size_t y = 0; y < kDimY; ++y) {
            CHECK(delays1[x][y] == delays2[x][y]);
        }
    }
}

TEST_CASE("round_trip_override_delay_model", "[vpr]") {
    constexpr size_t kDimX = 10;
    constexpr size_t kDimY = 10;
    vtr::Matrix<float> delays;
    delays.resize({kDimX, kDimY});

    for (size_t x = 0; x < kDimX; ++x) {
        for (size_t y = 0; y < kDimY; ++y) {
            delays[x][y] = (x + 1) * (y + 1);
        }
    }
    OverrideDelayModel model;
    auto base_model = std::make_unique<DeltaDelayModel>(delays);
    model.set_base_delay_model(std::move(base_model));
    model.set_delay_override(1, 2, 3, 4, 5, 6, -1);
    model.set_delay_override(2, 2, 3, 4, 5, 6, -2);

    model.write(kOverrideDelayBin);

    OverrideDelayModel model2;
    model2.read(kOverrideDelayBin);

    const auto& delays1 = model.base_delay_model()->delays();
    const auto& delays2 = model2.base_delay_model()->delays();

    REQUIRE(delays1.size() == delays2.size());
    REQUIRE(delays1.ndims() == delays2.ndims());
    for (size_t dim = 0; dim < delays1.ndims(); ++dim) {
        REQUIRE(delays1.dim_size(dim) == delays2.dim_size(dim));
    }

    for (size_t x = 0; x < kDimX; ++x) {
        for (size_t y = 0; y < kDimY; ++y) {
            CHECK(delays1[x][y] == delays2[x][y]);
        }
    }

    CHECK(model2.get_delay_override(1, 2, 3, 4, 5, 6) == -1);
    CHECK(model2.get_delay_override(2, 2, 3, 4, 5, 6) == -2);
}
#endif

} // namespace
