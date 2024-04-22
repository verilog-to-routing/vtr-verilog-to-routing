#ifndef NO_SERVER

#include "telegramparser.h"

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

TEST_CASE("test_server_telegram_parser_base", "[vpr]")
{
    const std::string tdata{R"({"JOB_ID":"7","CMD":"2","OPTIONS":"type1:name1:value1;type2:name2:v a l u e 2;t3:n3:v3","DATA":"some_data...","STATUS":"1"})"};

    REQUIRE(std::optional<int>{7} == comm::TelegramParser::try_extract_field_job_id(tdata));
    REQUIRE(std::optional<int>{2} == comm::TelegramParser::try_extract_field_cmd(tdata));
    REQUIRE(std::optional<std::string>{"type1:name1:value1;type2:name2:v a l u e 2;t3:n3:v3"} == comm::TelegramParser::try_extract_field_options(tdata));
    REQUIRE(std::optional<std::string>{"some_data..."} == comm::TelegramParser::try_extract_field_data(tdata));
    REQUIRE(std::optional<int>{1} == comm::TelegramParser::try_extract_field_status(tdata));
}

TEST_CASE("test_server_telegram_parser_invalid_keys", "[vpr]")
{
    const std::string tBadData{R"({"_JOB_ID":"7","_CMD":"2","_OPTIONS":"type1:name1:value1;type2:name2:v a l u e 2;t3:n3:v3","_DATA":"some_data...","_STATUS":"1"})"};
    
    REQUIRE(std::nullopt == comm::TelegramParser::try_extract_field_job_id(tBadData));
    REQUIRE(std::nullopt == comm::TelegramParser::try_extract_field_cmd(tBadData));
    REQUIRE(std::nullopt == comm::TelegramParser::try_extract_field_options(tBadData));
    REQUIRE(std::nullopt == comm::TelegramParser::try_extract_field_data(tBadData));
    REQUIRE(std::nullopt == comm::TelegramParser::try_extract_field_status(tBadData));
}

TEST_CASE("test_server_telegram_parser_invalid_types", "[vpr]")
{
    const std::string tBadData{R"({"JOB_ID":"x","CMD":"y","STATUS":"z"})"};
    
    REQUIRE(std::nullopt == comm::TelegramParser::try_extract_field_job_id(tBadData));
    REQUIRE(std::nullopt == comm::TelegramParser::try_extract_field_cmd(tBadData));
    REQUIRE(std::nullopt == comm::TelegramParser::try_extract_field_status(tBadData));
}

#endif /* NO_SERVER */