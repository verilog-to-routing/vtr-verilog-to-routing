#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "telegrambuffer.h"

namespace {

TEST_CASE("test_server_bytearray", "[vpr]") {
    comm::ByteArray array1{"111"};
    comm::ByteArray array2{"222"};
    comm::ByteArray array{array1};
    array.append(array2);

    REQUIRE(array.at(0) == '1');
    REQUIRE(array.at(1) == '1');
    REQUIRE(array.at(2) == '1');
    REQUIRE(array.at(3) == '2');
    REQUIRE(array.at(4) == '2');
    REQUIRE(array.at(5) == '2');

    REQUIRE(array.to_string() == "111222");

    REQUIRE(array.size() == 6);

    array.append('3');

    REQUIRE(array.size() == 7);
    REQUIRE(array.to_string() == "1112223");

    REQUIRE(array.at(6) == '3');

    array.clear();

    REQUIRE(array.size() == 0);
    REQUIRE(array.to_string() == "");
}

TEST_CASE("test_server_telegrambuffer_oneOpened", "[vpr]") {
    comm::TelegramBuffer buff{1024};
    buff.append(comm::ByteArray{"111"});
    buff.append(comm::ByteArray{"222"});

    auto frames = buff.takeFrames();
    REQUIRE(frames.size() == 0);

    REQUIRE(buff.data().to_string() == "111222");
}

TEST_CASE("test_server_telegrambuffer_oneFinishedOneOpened", "[vpr]") {
    comm::TelegramBuffer buff{1024};
    buff.append(comm::ByteArray{"111\x17"});
    buff.append(comm::ByteArray{"222"});

    auto frames = buff.takeFrames();
    REQUIRE(frames.size() == 1);

    REQUIRE(frames[0].to_string() == "111");

    REQUIRE(buff.data().to_string() == "222");
}

TEST_CASE("test_server_telegrambuffer_twoFinished", "[vpr]") {
    comm::TelegramBuffer buff{1024};
    buff.append(comm::ByteArray{"111\x17"});
    buff.append(comm::ByteArray{"222\x17"});

    auto frames = buff.takeFrames();
    REQUIRE(frames.size() == 2);

    REQUIRE(frames[0].to_string() == "111");
    REQUIRE(frames[1].to_string() == "222");

    REQUIRE(buff.data().to_string() == "");
}

TEST_CASE("test_server_telegrambuffer_twoCleared", "[vpr]") {
    comm::TelegramBuffer buff{1024};
    buff.append(comm::ByteArray{"111\x17"});
    buff.append(comm::ByteArray{"222\x17"});

    buff.clear();

    auto frames = buff.takeFrames();
    REQUIRE(frames.size() == 0);

    REQUIRE(buff.data().to_string() == "");
}

} // namespace