#ifndef TASKRESOLVER_H
#define TASKRESOLVER_H

#include "task.h"

#include <vector>

#ifndef STANDALONE_APP
namespace ezgl {
    class application;
}
#endif

class TaskResolver {
public:
    TaskResolver()=default;
    ~TaskResolver()=default;

    int tasksNum() const { return m_tasks.size(); }

    void addTasks(const std::vector<Task>&);
    void addTask(Task);
#ifndef STANDALONE_APP
    void update(ezgl::application*);
#else
    void update();
#endif
    void takeFinished(std::vector<Task>&);

    const std::vector<Task>& tasks() const { return m_tasks; }

private:
    std::vector<Task> m_tasks;
};

#endif
