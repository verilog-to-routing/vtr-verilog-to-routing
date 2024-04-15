#ifndef NO_SERVER

#ifndef TASKRESOLVER_H
#define TASKRESOLVER_H

#include "task.h"
#include "vpr_types.h"

#include <vector>

namespace ezgl {
    class application;
}

namespace server {

/**
 * @brief Resolve server task.
 * 
 * Process and resolve server task, store result and status for processed task.
*/

class TaskResolver {
public:
    TaskResolver()=default;
    ~TaskResolver()=default;

    int tasksNum() const { return m_tasks.size(); }

    /* add tasks to process */
    void addTask(TaskPtr&);
    void addTasks(std::vector<TaskPtr>&);

    /* process tasks */
    bool update(ezgl::application*);

    /* extract finished tasks */
    void takeFinished(std::vector<TaskPtr>&);

    const std::vector<TaskPtr>& tasks() const { return m_tasks; }

private:
    std::vector<TaskPtr> m_tasks;

    void processGetPathListTask(ezgl::application*, const TaskPtr&);
    void processDrawCriticalPathTask(ezgl::application*, const TaskPtr&);

    e_timing_report_detail getDetailsLevelEnum(const std::string& pathDetailsLevelStr) const;
};

} // namespace server

#endif /* TASKRESOLVER_H */

#endif /* NO_SERVER */