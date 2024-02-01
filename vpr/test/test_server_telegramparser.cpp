#include "telegramparser.h"

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

TEST_CASE("test_server_telegram_parser_base", "[vpr]") {
    const std::string tdata{R"({"JOB_ID":"7","CMD":"2","OPTIONS":"type1:name1:value1;type2:name2:v a l u e 2;t3:n3:v3","DATA":"some_data...","STATUS":"1"})"};

    REQUIRE(std::optional<int>{7} == comm::TelegramParser::tryExtractFieldJobId(tdata));
    REQUIRE(std::optional<int>{2} == comm::TelegramParser::tryExtractFieldCmd(tdata));
    REQUIRE(std::optional<std::string>{"type1:name1:value1;type2:name2:v a l u e 2;t3:n3:v3"} == comm::TelegramParser::tryExtractFieldOptions(tdata));
    REQUIRE(std::optional<std::string>{"some_data..."} == comm::TelegramParser::tryExtractFieldData(tdata));
    REQUIRE(std::optional<int>{1} == comm::TelegramParser::tryExtractFieldStatus(tdata));
}
