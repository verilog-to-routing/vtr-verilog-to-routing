#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "telegramoptions.h"

namespace {

TEST_CASE("test_server_telegramoptions", "[vpr]") {
    server::TelegramOptions options{"int:path_num:11;string:path_type:debug;int:details_level:3;bool:is_flat_routing:0", {"path_num", "path_type", "details_level", "is_flat_routing"}};

    REQUIRE(options.errorsStr() == "");

    REQUIRE(options.getString("path_type") == "debug");
    REQUIRE(options.getInt("path_num", -1) == 11);
    REQUIRE(options.getInt("details_level", -1) == 3);
    REQUIRE(options.getBool("is_flat_routing", true) == false);
}

} // namespace
