#ifndef TASKRESOLVER_H
#define TASKRESOLVER_H

#include "task.h"
#include "vpr_types.h"

#include <vector>

namespace ezgl {
    class application;
}

class TaskResolver {
public:
    TaskResolver()=default;
    ~TaskResolver()=default;

    int tasksNum() const { return m_tasks.size(); }

    void addTasks(const std::vector<Task>&);
    void addTask(Task);

    bool update(ezgl::application*);
    void takeFinished(std::vector<Task>&);

    const std::vector<Task>& tasks() const { return m_tasks; }

private:
    std::vector<Task> m_tasks;

    e_timing_report_detail getDetailsLevelEnum(const std::string& pathDetailsLevelStr) const ;
};

#endif
