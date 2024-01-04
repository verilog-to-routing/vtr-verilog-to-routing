#include "taskresolver.h"

#include "globals.h"
#include "pathhelper.h"
#include "telegramoptions.h"
#include "telegramparser.h"
#include "gtkcomboboxhelper.h"
#include <ezgl/application.hpp>

namespace server {

void TaskResolver::addTask(Task task)
{
    // pre-process task before adding, where we could quickly detect failure scenario
    for (auto& t: m_tasks) {
        if (t.cmd() == task.cmd()) {
            if (t.options() == task.options()) {
                std::string msg = "similar task is already in execution, reject new task: " + t.info()+ " and waiting for old task: " + task.info() + " execution";
                task.fail(msg);
            } else {
                // case when task has same jobId but different options
                if (task.jobId() > t.jobId()) {
                    std::string msg = "old task: " + t.info() + " is overriden by a new task: " + task.info();
                    t.fail(msg);
                }
            }
        }
    }

    // add task
    m_tasks.push_back(std::move(task));
}

void TaskResolver::addTasks(const std::vector<Task>& tasks)
{
    for (const Task& task: tasks) {
        addTask(task);
    }
}

void TaskResolver::takeFinished(std::vector<Task>& result)
{
    for (auto it=m_tasks.begin(); it != m_tasks.end();) {
        Task task = *it;
        if (task.isFinished()) {
            result.push_back(std::move(task));
            it = m_tasks.erase(it);
        } else {
            ++it;
        }
    }
}

e_timing_report_detail TaskResolver::getDetailsLevelEnum(const std::string& pathDetailsLevelStr) const {
    e_timing_report_detail detailesLevel = e_timing_report_detail::NETLIST;
    if (pathDetailsLevelStr == "netlist") {
        detailesLevel = e_timing_report_detail::NETLIST;
    } else if (pathDetailsLevelStr == "aggregated") {
        detailesLevel = e_timing_report_detail::AGGREGATED;
    } else if (pathDetailsLevelStr == "detailed") {
        detailesLevel = e_timing_report_detail::DETAILED_ROUTING;
    } else if (pathDetailsLevelStr == "debug") {
        detailesLevel = e_timing_report_detail::DEBUG;
    } else {
        std::cerr << "unhandled option" << pathDetailsLevelStr << std::endl;
    }
    return detailesLevel;
}

bool TaskResolver::update(ezgl::application* app)
{
    bool has_processed_task = false;
    for (auto& task: m_tasks) {
        if (!task.isFinished()) {
            switch(task.cmd()) {
                case CMD_GET_PATH_LIST_ID: {
                    processGetPathListTask(app, task);
                    has_processed_task = true;
                    break;
                } 
                case CMD_DRAW_PATH_ID: {
                    processDrawCriticalPathTask(app, task);
                    has_processed_task = true;
                    break;
                }
            }           
        }
    }

    return has_processed_task;
}

void TaskResolver::processGetPathListTask(ezgl::application* app, Task& task)
{
    TelegramOptions options{task.options(), {OPTION_PATH_NUM, OPTION_PATH_TYPE, OPTION_DETAILS_LEVEL, OPTION_IS_FLOAT_ROUTING}};
    if (!options.hasErrors()) {
        ServerContext& server_ctx = g_vpr_ctx.mutable_server(); // shortcut

        server_ctx.set_crit_path_index(-1); // reset selection if path list options has changed

        // read options
        const int nCriticalPathNum = options.getInt(OPTION_PATH_NUM, 1);
        const std::string pathType = options.getString(OPTION_PATH_TYPE);
        const std::string detailsLevel = options.getString(OPTION_DETAILS_LEVEL);
        const bool isFlat = options.getBool(OPTION_IS_FLOAT_ROUTING, false);

        // calculate critical path depending on options and store result in server context
        CritPathsResult crit_paths_result = calcCriticalPath(pathType, nCriticalPathNum, getDetailsLevelEnum(detailsLevel), isFlat);

        // setup context
        server_ctx.set_path_type(pathType);
        server_ctx.set_critical_path_num(nCriticalPathNum);
        server_ctx.set_crit_paths(crit_paths_result.paths);

        if (crit_paths_result.isValid()) {
            std::string msg{crit_paths_result.report};
            task.success(msg);
        } else {
            std::string msg{"Critical paths report is empty"};
            std::cerr << msg << std::endl;
            task.fail(msg);
        }
    } else {
        std::string msg{"options errors in get crit path list telegram: " + options.errorsStr()};
        std::cerr << msg << std::endl;
        task.fail(msg);
    }
}

void TaskResolver::processDrawCriticalPathTask(ezgl::application* app, Task& task)
{
    TelegramOptions options{task.options(), {OPTION_PATH_INDEX, OPTION_HIGHTLIGHT_MODE}};
    if (!options.hasErrors()) {
        ServerContext& server_ctx = g_vpr_ctx.mutable_server(); // shortcut

        const int pathIndex = options.getInt(OPTION_PATH_INDEX, -1);
        const std::string highLightMode = options.getString(OPTION_HIGHTLIGHT_MODE);

        if ((pathIndex >= 0) && (pathIndex < static_cast<int>(server_ctx.crit_paths().size()))) {
            // set critical path index for rendering
            server_ctx.set_crit_path_index(pathIndex);

            // update gtk UI
            GtkComboBox* toggle_crit_path = GTK_COMBO_BOX(app->get_object("ToggleCritPath"));
            gint highLightModeIndex = get_item_index_by_text(toggle_crit_path, highLightMode.c_str());
            if (highLightModeIndex != -1) {
                gtk_combo_box_set_active(toggle_crit_path, highLightModeIndex);
                task.success();
            } else {
                std::string msg{"cannot find ToggleCritPath qcombobox index for item " + highLightMode};
                std::cerr << msg << std::endl;
                task.fail(msg);
            }                        
        } else {
            std::string msg{"selectedIndex=" + std::to_string(pathIndex) + " is out of range [0-" + std::to_string(static_cast<int>(server_ctx.crit_paths().size())-1) + "]"};
            std::cerr << msg << std::endl;
            task.fail(msg);
        }
    } else {
        std::string msg{"options errors in highlight crit path telegram: " + options.errorsStr()};
        std::cerr << msg << std::endl;
        task.fail(msg);
    }
}

} // namespace server
