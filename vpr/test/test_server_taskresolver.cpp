#ifndef NO_SERVER

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "taskresolver.h"
#include <memory>

TEST_CASE("test_server_taskresolver_cmdSpamFilter", "[vpr]") {
    server::TaskResolver resolver;
    const comm::CMD cmd = comm::CMD::GET_PATH_LIST_ID;

    {
        server::TaskPtr task0 = std::make_unique<server::Task>(1, cmd);
        server::TaskPtr task1 = std::make_unique<server::Task>(2, cmd);
        server::TaskPtr task2 = std::make_unique<server::Task>(3, cmd);
        server::TaskPtr task3 = std::make_unique<server::Task>(4, cmd);
        server::TaskPtr task4 = std::make_unique<server::Task>(5, cmd);

        resolver.own_task(std::move(task0));
        resolver.own_task(std::move(task1));
        resolver.own_task(std::move(task2));
        resolver.own_task(std::move(task3));
        resolver.own_task(std::move(task4));
    }

    std::vector<server::TaskPtr> finished;
    resolver.take_finished_tasks(finished);

    REQUIRE(finished.size() == 4);

    for (const auto& task: finished) {
        REQUIRE(task->is_finished());
        REQUIRE(task->has_error());
        REQUIRE(task->job_id() != 1);
        REQUIRE(task->cmd() == cmd);
    }
    REQUIRE(resolver.tasks_num() == 1);
    const server::TaskPtr& task = resolver.tasks().at(0);
    REQUIRE(task->job_id() == 1);
    REQUIRE(task->cmd() == cmd);
}

TEST_CASE("test_server_taskresolver_cmdOverrideFilter", "[vpr]") {
    server::TaskResolver resolver;
    const comm::CMD cmd = comm::CMD::GET_PATH_LIST_ID;

    {
        server::TaskPtr task0 = std::make_unique<server::Task>(1, cmd, "1");
        server::TaskPtr task1 = std::make_unique<server::Task>(2, cmd, "11");
        server::TaskPtr task2 = std::make_unique<server::Task>(3, cmd, "222");

        resolver.own_task(std::move(task0));
        resolver.own_task(std::move(task1));
        resolver.own_task(std::move(task2));
    }

    std::vector<server::TaskPtr> finished;
    resolver.take_finished_tasks(finished);

    REQUIRE(finished.size() == 2);

    for (const server::TaskPtr& task: finished) {
        REQUIRE(task->is_finished());
        REQUIRE(task->has_error());
        REQUIRE(task->job_id() != 3);
    }
    REQUIRE(resolver.tasks_num() == 1);
    const server::TaskPtr& task = resolver.tasks().at(0);
    REQUIRE(task->job_id() == 3);
    REQUIRE(task->cmd() == cmd);
    REQUIRE(task->options() == "222");
}

TEST_CASE("test_server_taskresolver_cmdSpamAndOverrideOptions", "[vpr]") {
    server::TaskResolver resolver;

    const comm::CMD cmd1 = comm::CMD::GET_PATH_LIST_ID;
    const comm::CMD cmd2 = comm::CMD::DRAW_PATH_ID;

    {
        server::TaskPtr task0 = std::make_unique<server::Task>(1, cmd2, "1");
        server::TaskPtr task1 = std::make_unique<server::Task>(2, cmd2, "11");
        server::TaskPtr task2 = std::make_unique<server::Task>(3, cmd2, "222");
        server::TaskPtr task3 = std::make_unique<server::Task>(4, cmd2, "222");
        server::TaskPtr task4 = std::make_unique<server::Task>(5, cmd1);
        server::TaskPtr task5 = std::make_unique<server::Task>(6, cmd1);
        server::TaskPtr task6 = std::make_unique<server::Task>(7, cmd1);

        resolver.own_task(std::move(task0));
        resolver.own_task(std::move(task1));
        resolver.own_task(std::move(task2));
        resolver.own_task(std::move(task3));
        resolver.own_task(std::move(task4));
        resolver.own_task(std::move(task5));
        resolver.own_task(std::move(task6));
    }

    std::vector<server::TaskPtr> finished;
    resolver.take_finished_tasks(finished);

    REQUIRE(resolver.tasks_num() == 2);
    const server::TaskPtr& task0 = resolver.tasks().at(0);
    const server::TaskPtr& task1 = resolver.tasks().at(1);

    REQUIRE(task0->job_id() == 3);
    REQUIRE(task0->cmd() == cmd2);
    REQUIRE(task0->options() == "222");

    REQUIRE(task1->job_id() == 5);
    REQUIRE(task1->cmd() == cmd1);
    REQUIRE(task1->options() == "");
}

#endif /* NO_SERVER */