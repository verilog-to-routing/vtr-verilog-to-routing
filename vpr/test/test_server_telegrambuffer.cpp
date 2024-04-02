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
    comm::TelegramBuffer buff;
    buff.append(comm::ByteArray{"111"});
    buff.append(comm::ByteArray{"222"});

    auto frames = buff.takeTelegramFrames();
    REQUIRE(frames.size() == 0);

    REQUIRE(buff.data().to_string() == "111222");
}

TEST_CASE("test_server_telegrambuffer_notFilledTelegramButWithPrependedRubish", "[vpr]")
{
    comm::TelegramBuffer tBuff;

    const comm::ByteArray rubish{"#@!"};
    const comm::ByteArray msgBody{"some message"};
    const comm::TelegramHeader msgHeader{comm::TelegramHeader::constructFromData(msgBody)};

    tBuff.append(rubish);
    tBuff.append(msgHeader.buffer());

    auto frames = tBuff.takeTelegramFrames();
    REQUIRE(0 == frames.size());

    REQUIRE(msgHeader.buffer() == tBuff.data()); // the rubish prefix fragment will be absent here
}

TEST_CASE("test_server_telegrambuffer__oneFinishedOneOpened", "[vpr]")
{
    comm::TelegramBuffer tBuff;

    const comm::ByteArray msgBody1{"message1"};
    const comm::ByteArray msgBody2{"message2"};

    const comm::TelegramHeader msgHeader1{comm::TelegramHeader::constructFromData(msgBody1)};
    const comm::TelegramHeader msgHeader2{comm::TelegramHeader::constructFromData(msgBody2)};

    comm::ByteArray t1(msgHeader1.buffer());
    t1.append(msgBody1);

    comm::ByteArray t2(msgHeader2.buffer());
    t2.append(msgBody2);
    t2.resize(t2.size()-2); // drop 2 last elements

    tBuff.append(t1);
    tBuff.append(t2);

    auto frames = tBuff.takeTelegramFrames();
    REQUIRE(1 == frames.size());

    REQUIRE(msgBody1 == frames[0]->data);

    REQUIRE(t2 == tBuff.data());
}

TEST_CASE("test_server_telegrambuffer_twoFinished", "[vpr]")
{
    comm::TelegramBuffer tBuff;

    const comm::ByteArray msgBody1{"message1"};
    const comm::ByteArray msgBody2{"message2"};

    const comm::TelegramHeader msgHeader1{comm::TelegramHeader::constructFromData(msgBody1)};
    const comm::TelegramHeader msgHeader2{comm::TelegramHeader::constructFromData(msgBody2)};

    comm::ByteArray t1(msgHeader1.buffer());
    t1.append(msgBody1);

    comm::ByteArray t2(msgHeader2.buffer());
    t2.append(msgBody2);

    tBuff.append(t1);
    tBuff.append(t2);

    auto frames = tBuff.takeTelegramFrames();
    REQUIRE(2 == frames.size());

    REQUIRE(msgBody1 == frames[0]->data);
    REQUIRE(msgBody2 == frames[1]->data);

    REQUIRE(comm::ByteArray{} == tBuff.data());
}

TEST_CASE("test_server_telegrambuffer_clear", "[vpr]")
{
    comm::TelegramBuffer tBuff;

    const comm::ByteArray msgBody1{"message1"};
    const comm::ByteArray msgBody2{"message2"};

    const comm::TelegramHeader msgHeader1{comm::TelegramHeader::constructFromData(msgBody1)};
    const comm::TelegramHeader msgHeader2{comm::TelegramHeader::constructFromData(msgBody2)};

    comm::ByteArray t1(msgHeader1.buffer());
    t1.append(msgBody1);

    comm::ByteArray t2(msgHeader2.buffer());
    t2.append(msgBody2);

    tBuff.clear();

    auto frames = tBuff.takeTelegramFrames();
    REQUIRE(0 == frames.size());

    REQUIRE(comm::ByteArray{} == tBuff.data());
}

} // namespace
