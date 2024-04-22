#ifndef NO_SERVER

#ifndef TASKRESOLVER_H
#define TASKRESOLVER_H

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
    TaskResolver()=default;
    ~TaskResolver()=default;

    int tasks_num() const { return m_tasks.size(); }

    /* own task to process */
    void own_task(TaskPtr&&);

    /* process tasks */
    bool update(ezgl::application*);

    /* extract finished tasks */
    void take_finished_tasks(std::vector<TaskPtr>&);

    const std::vector<TaskPtr>& tasks() const { return m_tasks; }

private:
    std::vector<TaskPtr> m_tasks;

    void process_get_path_list_task(ezgl::application*, const TaskPtr&);
    void process_draw_critical_path_task(ezgl::application*, const TaskPtr&);

    std::optional<e_timing_report_detail> try_get_details_level_enum(const std::string& path_details_level_str) const;
};

} // namespace server

#endif /* TASKRESOLVER_H */

#endif /* NO_SERVER */
