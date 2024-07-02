#ifndef NO_SERVER

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "telegrambuffer.h"

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

    REQUIRE(std::string_view{array} == "111222");

    REQUIRE(array.size() == 6);

    array.append('3');

    REQUIRE(array.size() == 7);
    REQUIRE(std::string_view{array} == "1112223");

    REQUIRE(array.at(6) == '3');

    array.clear();

    REQUIRE(array.size() == 0);
    REQUIRE(std::string_view{array} == "");
}

TEST_CASE("test_server_telegrambuffer_oneOpened", "[vpr]") {
    comm::TelegramBuffer buff;
    buff.append(comm::ByteArray{"111"});
    buff.append(comm::ByteArray{"222"});

    std::vector<comm::TelegramFramePtr> frames;
    buff.take_telegram_frames(frames);
    REQUIRE(frames.size() == 0);

    REQUIRE(std::string_view{buff.data()} == "111222");
}

TEST_CASE("test_server_telegrambuffer_notFilledTelegramButWithPrependedRubish", "[vpr]")
{
    comm::TelegramBuffer tBuff;

    const comm::ByteArray rubbish{"#@!"};
    const comm::ByteArray msgBody{"some message"};
    const comm::TelegramHeader msgHeader{comm::TelegramHeader::construct_from_body(msgBody)};

    tBuff.append(rubbish);
    tBuff.append(msgHeader.buffer());

    std::vector<comm::TelegramFramePtr> frames;
    tBuff.take_telegram_frames(frames);
    REQUIRE(0 == frames.size());

    REQUIRE(msgHeader.buffer() == tBuff.data()); // the rubbish prefix fragment will be absent here
}

TEST_CASE("test_server_telegrambuffer__oneFinishedOneOpened", "[vpr]")
{
    comm::TelegramBuffer tBuff;

    const comm::ByteArray msgBody1{"message1"};
    const comm::ByteArray msgBody2{"message2"};

    const comm::TelegramHeader msgHeader1{comm::TelegramHeader::construct_from_body(msgBody1)};
    const comm::TelegramHeader msgHeader2{comm::TelegramHeader::construct_from_body(msgBody2)};

    comm::ByteArray t1(msgHeader1.buffer());
    t1.append(msgBody1);

    comm::ByteArray t2(msgHeader2.buffer());
    t2.append(msgBody2);
    t2.resize(t2.size()-2); // drop 2 last elements

    tBuff.append(t1);
    tBuff.append(t2);

    std::vector<comm::TelegramFramePtr> frames;
    tBuff.take_telegram_frames(frames);
    REQUIRE(1 == frames.size());

    REQUIRE(msgBody1 == frames[0]->body);

    REQUIRE(t2 == tBuff.data());
}

TEST_CASE("test_server_telegrambuffer_twoFinished", "[vpr]")
{
    comm::TelegramBuffer tBuff;

    const comm::ByteArray msgBody1{"message1"};
    const comm::ByteArray msgBody2{"message2"};

    const comm::TelegramHeader msgHeader1{comm::TelegramHeader::construct_from_body(msgBody1)};
    const comm::TelegramHeader msgHeader2{comm::TelegramHeader::construct_from_body(msgBody2)};

    comm::ByteArray t1(msgHeader1.buffer());
    t1.append(msgBody1);

    comm::ByteArray t2(msgHeader2.buffer());
    t2.append(msgBody2);

    tBuff.append(t1);
    tBuff.append(t2);

    std::vector<comm::TelegramFramePtr> frames;
    tBuff.take_telegram_frames(frames);
    REQUIRE(2 == frames.size());

    REQUIRE(msgBody1 == frames[0]->body);
    REQUIRE(msgBody2 == frames[1]->body);

    REQUIRE(comm::ByteArray{} == tBuff.data());
}

TEST_CASE("test_server_telegrambuffer_clear", "[vpr]")
{
    comm::TelegramBuffer tBuff;

    const comm::ByteArray msgBody1{"message1"};
    const comm::ByteArray msgBody2{"message2"};

    const comm::TelegramHeader msgHeader1{comm::TelegramHeader::construct_from_body(msgBody1)};
    const comm::TelegramHeader msgHeader2{comm::TelegramHeader::construct_from_body(msgBody2)};

    comm::ByteArray t1(msgHeader1.buffer());
    t1.append(msgBody1);

    comm::ByteArray t2(msgHeader2.buffer());
    t2.append(msgBody2);

    tBuff.clear();

    std::vector<comm::TelegramFramePtr> frames;
    tBuff.take_telegram_frames(frames);
    REQUIRE(0 == frames.size());

    REQUIRE(comm::ByteArray{} == tBuff.data());
}

#endif /* NO_SERVER */