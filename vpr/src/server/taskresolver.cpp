#include "taskresolver.h"

#include "globals.h"
#include "pathhelper.h"
#include "telegramoptions.h"
#include "telegramparser.h"
#include "gtkcomboboxhelper.h"
#include <ezgl/application.hpp>

void TaskResolver::addTasks(const std::vector<Task>& tasks)
{
    for (const Task& task: tasks) {
        addTask(task);
    }
}

void TaskResolver::addTask(Task task)
{
    for (auto& t: m_tasks) {
        if (t.cmd() == task.cmd()) {
            if (t.options() == task.options()) {
                std::string msg = "similar task is already in execution, reject new task: " + t.info()+ " and waiting for old task: " + task.info() + " execution";
                task.fail(msg);
            } else {
                if (task.jobId() > t.jobId()) {
                    std::string msg = "old task: " + t.info() + " is overriden by a new task: " + task.info();
                    t.fail(msg);
                }
            }
        }
    }
    m_tasks.push_back(std::move(task));
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

bool TaskResolver::update(ezgl::application* app)
{
    bool process_task = false;
    ServerContext& server_ctx = g_vpr_ctx.server_ctx();
    for (auto& task: m_tasks) {
        if (!task.isFinished()) {
            process_task = true;
            TelegramOptions options{task.options()};
            if (task.cmd() == CMD_GET_PATH_LIST_ID) {
                options.validateNamePresence({OPTION_PATH_NUM, OPTION_PATH_TYPE, OPTION_DETAILS_LEVEL, OPTION_IS_FLOAT_ROUTING});
                int nCriticalPathNum = options.getInt(OPTION_PATH_NUM, 1);
                std::string pathType = options.getString(OPTION_PATH_TYPE);
                std::string detailsLevel = options.getString(OPTION_DETAILS_LEVEL);
                bool isFlat = options.getBool(OPTION_IS_FLOAT_ROUTING, false);
                if (!options.hasErrors()) {
                    if (pathType != server_ctx.path_type()) {
                        server_ctx.set_crit_path_index(-1);
                    }

                    server_ctx.set_path_type(pathType);
                    server_ctx.set_critical_path_num(nCriticalPathNum);
                    if (pathType == "setup") {
                        auto paths = calcSetupCritPaths(nCriticalPathNum);
                        server_ctx.set_crit_paths(paths);
                    } else if (pathType == "hold") {
                        auto hold_timing_info = server_ctx.hold_timing_info();
                        if (hold_timing_info) {
                            auto paths = calcHoldCritPaths(nCriticalPathNum, hold_timing_info);
                            server_ctx.set_crit_paths(paths);
                        } else {
                            std::string msg{"cannot calculate hold critical path due to hold_timing_info nullptr"};
                            std::cerr << msg << std::endl;
                            task.fail(msg);
                        }
                    } else {
                        std::string msg{"unknown path type " + pathType};
                        std::cerr << msg << std::endl;
                        task.fail(msg);
                    }

                    if (!task.hasError()) {
                        std::string msg{getPathsStr(server_ctx.crit_paths(), detailsLevel, isFlat)};
                        task.success(msg);
                    }
                } else {
                    std::string msg{"options errors in get crit path list telegram: " + options.errorsStr()};
                    std::cerr << msg << std::endl;
                    task.fail(msg);
                }
            } else if (task.cmd() == CMD_DRAW_PATH_ID) {
                options.validateNamePresence({OPTION_PATH_INDEX, OPTION_HIGHTLIGHT_MODE});
                int pathIndex = options.getInt(OPTION_PATH_INDEX, -1);
                std::string highLightMode = options.getString(OPTION_HIGHTLIGHT_MODE);
                if (!options.hasErrors()) {
                    if ((pathIndex >= 0) && (pathIndex < static_cast<int>(server_ctx.crit_paths().size()))) {
                        server_ctx.set_crit_path_index(pathIndex);

                        // update gtk UI
                        GtkComboBox* toggle_crit_path = GTK_COMBO_BOX(app->get_object("ToggleCritPath"));
                        gint highLightModeIndex = get_item_index_by_text(toggle_crit_path, highLightMode.data());
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
        }
    }

    return process_task;
}

