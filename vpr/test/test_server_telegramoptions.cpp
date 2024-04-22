#ifndef NO_SERVER

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "telegramoptions.h"

TEST_CASE("test_server_telegramoptions", "[vpr]") {
    server::TelegramOptions options{"int:path_num:11;string:path_type:debug;int:details_level:3;bool:is_flat_routing:0", {"path_num", "path_type", "details_level", "is_flat_routing"}};

    REQUIRE(options.errors_str() == "");

    REQUIRE(options.get_string("path_type") == "debug");
    REQUIRE(options.get_int("path_num", -1) == 11);
    REQUIRE(options.get_int("details_level", -1) == 3);
    REQUIRE(options.get_bool("is_flat_routing", true) == false);
}

TEST_CASE("test_server_telegramoptions_get_wrong_keys", "[vpr]") {
    server::TelegramOptions options{"int:path_num:11;string:path_type:debug;int:details_level:3;bool:is_flat_routing:0", {"_path_num", "_path_type", "_details_level", "_is_flat_routing"}};

    REQUIRE(!options.errors_str().empty());

    REQUIRE(options.get_string("_path_type") == "");
    REQUIRE(options.get_int("_path_num", -1) == -1);
    REQUIRE(options.get_int("_details_level", -1) == -1);
    REQUIRE(options.get_bool("_is_flat_routing", true) == true);
}

#endif /* NO_SERVER */