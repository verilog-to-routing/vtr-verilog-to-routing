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
    TaskResolver() = default;
    ~TaskResolver() = default;

    int tasksNum() const { return m_tasks.size(); }

    /* add tasks to process */
    void addTask(Task);
    void addTasks(const std::vector<Task>&);

    /* process tasks */
    bool update(ezgl::application*);

    /* extract finished tasks */
    void takeFinished(std::vector<Task>&);

    const std::vector<Task>& tasks() const { return m_tasks; }

  private:
    std::vector<Task> m_tasks;

    void processGetPathListTask(ezgl::application*, Task&);
    void processDrawCriticalPathTask(ezgl::application*, Task&);

    e_timing_report_detail getDetailsLevelEnum(const std::string& pathDetailsLevelStr) const;
};

} // namespace server

#endif // TASKRESOLVER_H
