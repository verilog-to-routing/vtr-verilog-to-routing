#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "taskresolver.h"
#include <memory>

#ifndef NO_GRAPHICS

TEST_CASE("test_server_taskresolver_cmdSpamFilter", "[vpr]") {
    server::TaskResolver resolver;
    const int cmd = 10;

    {
        server::TaskPtr task0 = std::make_unique<server::Task>(1,cmd);
        server::TaskPtr task1 = std::make_unique<server::Task>(2,cmd);
        server::TaskPtr task2 = std::make_unique<server::Task>(3,cmd);
        server::TaskPtr task3 = std::make_unique<server::Task>(4,cmd);
        server::TaskPtr task4 = std::make_unique<server::Task>(5,cmd);

        resolver.addTask(task0);
        resolver.addTask(task1);
        resolver.addTask(task2);
        resolver.addTask(task3);
        resolver.addTask(task4);
    }

    std::vector<server::TaskPtr> finished;
    resolver.takeFinished(finished);

    REQUIRE(finished.size() == 4);

    for (const auto& task: finished) {
        REQUIRE(task->isFinished());
        REQUIRE(task->hasError());
        REQUIRE(task->jobId() != 1);
        REQUIRE(task->cmd() == cmd);
    }
    REQUIRE(resolver.tasksNum() == 1);
    const server::TaskPtr& task = resolver.tasks().at(0);
    REQUIRE(task->jobId() == 1);
    REQUIRE(task->cmd() == cmd);
}

TEST_CASE("test_server_taskresolver_cmdOverrideFilter", "[vpr]") {
    server::TaskResolver resolver;
    const int cmd = 10;

    {
        server::TaskPtr task0 = std::make_unique<server::Task>(1,cmd,"1");
        server::TaskPtr task1 = std::make_unique<server::Task>(2,cmd,"11");
        server::TaskPtr task2 = std::make_unique<server::Task>(3,cmd,"222");

        resolver.addTask(task0);
        resolver.addTask(task1);
        resolver.addTask(task2);
    }

    std::vector<server::TaskPtr> finished;
    resolver.takeFinished(finished);

    REQUIRE(finished.size() == 2);

    for (const server::TaskPtr& task: finished) {
        REQUIRE(task->isFinished());
        REQUIRE(task->hasError());
        REQUIRE(task->jobId() != 3);
    }
    REQUIRE(resolver.tasksNum() == 1);
    const server::TaskPtr& task = resolver.tasks().at(0);
    REQUIRE(task->jobId() == 3);
    REQUIRE(task->cmd() == cmd);
    REQUIRE(task->options() == "222");
}

TEST_CASE("test_server_taskresolver_cmdSpamAndOverrideOptions", "[vpr]") {
    server::TaskResolver resolver;

    {
        server::TaskPtr task0 = std::make_unique<server::Task>(1,2,"1");
        server::TaskPtr task1 = std::make_unique<server::Task>(2,2,"11");
        server::TaskPtr task2 = std::make_unique<server::Task>(3,2,"222");
        server::TaskPtr task3 = std::make_unique<server::Task>(4,2,"222");
        server::TaskPtr task4 = std::make_unique<server::Task>(5,1);
        server::TaskPtr task5 = std::make_unique<server::Task>(6,1);
        server::TaskPtr task6 = std::make_unique<server::Task>(7,1);

        resolver.addTask(task0);
        resolver.addTask(task1);
        resolver.addTask(task2);
        resolver.addTask(task3);
        resolver.addTask(task4);
        resolver.addTask(task5);
        resolver.addTask(task6);
    }

    std::vector<server::TaskPtr> finished;
    resolver.takeFinished(finished);

    REQUIRE(resolver.tasksNum() == 2);
    const server::TaskPtr& task0 = resolver.tasks().at(0);
    const server::TaskPtr& task1 = resolver.tasks().at(1);

    REQUIRE(task0->jobId() == 3);
    REQUIRE(task0->cmd() == 2);
    REQUIRE(task0->options() == "222");

    REQUIRE(task1->jobId() == 5);
    REQUIRE(task1->cmd() == 1);
    REQUIRE(task1->options() == "");
}

#endif /* NO_GRAPHICS */