#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "telegramoptions.h"

TEST_CASE("test_server_telegramoptions", "[vpr]") {
    server::TelegramOptions options{"int:path_num:11;string:path_type:debug;int:details_level:3;bool:is_flat_routing:0", {"path_num", "path_type", "details_level", "is_flat_routing"}};

    REQUIRE(options.errorsStr() == "");

    REQUIRE(options.getString("path_type") == "debug");
    REQUIRE(options.getInt("path_num", -1) == 11);
    REQUIRE(options.getInt("details_level", -1) == 3);
    REQUIRE(options.getBool("is_flat_routing", true) == false);
}

TEST_CASE("test_server_telegramoptions_get_wrong_keys", "[vpr]") {
    server::TelegramOptions options{"int:path_num:11;string:path_type:debug;int:details_level:3;bool:is_flat_routing:0", {"_path_num", "_path_type", "_details_level", "_is_flat_routing"}};

    REQUIRE(!options.errorsStr().empty());

    REQUIRE(options.getString("_path_type") == "");
    REQUIRE(options.getInt("_path_num", -1) == -1);
    REQUIRE(options.getInt("_details_level", -1) == -1);
    REQUIRE(options.getBool("_is_flat_routing", true) == true);
}
