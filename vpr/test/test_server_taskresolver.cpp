#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "taskresolver.h"

namespace {

TEST_CASE("test_server_taskresolver_cmdSpamFilter", "[vpr]") {
    server::TaskResolver resolver;
    resolver.addTask(server::Task{1,1});
    resolver.addTask(server::Task{2,1});
    resolver.addTask(server::Task{3,1});
    resolver.addTask(server::Task{4,1});
    resolver.addTask(server::Task{5,1});

    std::vector<server::Task> finished;
    resolver.takeFinished(finished);

    REQUIRE(finished.size() == 4);

    for (const auto& task: finished) {
        REQUIRE(task.isFinished());
        REQUIRE(task.hasError());
        REQUIRE(task.jobId() != 1);
    }
    REQUIRE(resolver.tasksNum() == 1);
    server::Task task = resolver.tasks()[0];
    REQUIRE(task.jobId()==1);
    REQUIRE(task.cmd()==1);
}

TEST_CASE("test_server_taskresolver_cmdOverrideFilter", "[vpr]") {
    server::TaskResolver resolver;
    resolver.addTask(server::Task{1,2,"1"});
    resolver.addTask(server::Task{2,2,"11"});
    resolver.addTask(server::Task{3,2,"222"});

    std::vector<server::Task> finished;
    resolver.takeFinished(finished);

    REQUIRE(finished.size() == 2);

    for (const auto& task: finished) {
        REQUIRE(task.isFinished());
        REQUIRE(task.hasError());
        REQUIRE(task.jobId() != 3);
    }
    REQUIRE(resolver.tasksNum() == 1);
    server::Task task = resolver.tasks()[0];
    REQUIRE(task.jobId()==3);
    REQUIRE(task.cmd()==2);
    REQUIRE(task.options()=="222");
}

TEST_CASE("test_server_taskresolver_cmdSpamAndOverrideOptions", "[vpr]") {
    server::TaskResolver resolver;
    resolver.addTask(server::Task{1,2,"1"});
    resolver.addTask(server::Task{2,2,"11"});
    resolver.addTask(server::Task{3,2,"222"});
    resolver.addTask(server::Task{4,2,"222"});
    resolver.addTask(server::Task{5,1});
    resolver.addTask(server::Task{6,1});
    resolver.addTask(server::Task{7,1});

    std::vector<server::Task> finished;
    resolver.takeFinished(finished);

    REQUIRE(resolver.tasksNum() == 2);
    server::Task task0 = resolver.tasks()[0];
    server::Task task1 = resolver.tasks()[1];

    REQUIRE(task0.jobId()==3);
    REQUIRE(task0.cmd()==2);
    REQUIRE(task0.options()=="222");

    REQUIRE(task1.jobId()==5);
    REQUIRE(task1.cmd()==1);
    REQUIRE(task1.options()=="");
}

} // namespace