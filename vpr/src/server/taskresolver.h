#ifndef TASKRESOLVER_H
#define TASKRESOLVER_H

#ifndef NO_SERVER

#include "task.h"
#include "vpr_types.h"

#include <vector>
#include <optional>

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
    /**
     * @brief Default constructor for TaskResolver.
     */
    TaskResolver()=default;

    ~TaskResolver()=default;

    int tasks_num() const { return m_tasks.size(); }

    /**
    * @brief Takes ownership of a task.
    *
    * This method takes ownership of a task by moving it into the TaskResolver's internal task queue.
    * After calling this method, the task will be owned and managed by the TaskResolver.
    *
    * @param task The task to take ownership of. After calling this method, the task object will be in a valid but unspecified state.
    *
    * @note After calling this method, the caller should avoid accessing or modifying the task object.
    */ 
    void own_task(TaskPtr&& task);

    /**
    * @brief Resolve queued tasks.
    *
    * @param app A pointer to the ezgl::application object representing the application instance.
    */
    bool update(ezgl::application* app);

    /**
    * @brief Extracts finished tasks from the internal task queue.
    *
    * This function removes finished tasks from the internal task queue and appends them to the provided vector.
    * After this operation, the internal task queue will no longer hold the extracted tasks.
    *
    * @param tasks A reference to a vector where the finished tasks will be appended.
    */
    void take_finished_tasks(std::vector<TaskPtr>& tasks);

    // helper method used in tests
    const std::vector<TaskPtr>& tasks() const { return m_tasks; }

private:
    std::vector<TaskPtr> m_tasks;

    void process_get_path_list_task(ezgl::application*, const TaskPtr&);
    void process_draw_critical_path_task(ezgl::application*, const TaskPtr&);

    std::optional<e_timing_report_detail> try_get_details_level_enum(const std::string& path_details_level_str) const;
};

} // namespace server

#endif /* NO_SERVER */

#endif /* TASKRESOLVER_H */

