#include "taskresolver.h"
#ifdef ENABLE_RANDOM_PATH_GENERATION
#include <server_app/pathsgen.h>
#endif

#ifndef STANDALONE_APP
#include "globals.h"
#include "pathhelper.h"
#include "telegramoptions.h"
#include "telegramparser.h"
#include "gtkcomboboxhelper.h"
#include <ezgl/application.hpp>
#endif

#ifdef ENABLE_TASK_DELAY
#include <thread>
#include <chrono>
#endif

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

std::vector<std::string> splitString(const std::string& input, char delimiter) 
{
    std::vector<std::string> tokens;
    std::istringstream tokenStream(input);
    std::string token;

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

#ifndef STANDALONE_APP
void TaskResolver::update(ezgl::application* app)
{
    for (auto& task: m_tasks) {
        if (!task.isFinished()) {
            TelegramOptions options{task.options()};
            if (task.cmd() == CMD_GET_PATH_LIST_ID) {
                options.validateNamePresence({OPTION_PATH_NUM, OPTION_PATH_TYPE, OPTION_DETAILS_LEVEL, OPTION_IS_FLOAT_ROUTING});
                int nCriticalPathNum = options.getInt(OPTION_PATH_NUM, 1);
                std::string typePath = options.getString(OPTION_PATH_TYPE);
                std::string detailsLevel = options.getString(OPTION_DETAILS_LEVEL);
                bool isFlat = options.getBool(OPTION_IS_FLOAT_ROUTING, false);
                if (!options.hasErrors()) {
                    if (typePath != g_vpr_ctx.path_type) {
                        g_vpr_ctx.crit_path_index = -1;
                    }
                    calcCritPath(nCriticalPathNum, typePath);
                    std::string msg = getPathsStr(g_vpr_ctx.crit_paths, detailsLevel, isFlat);
                    g_vpr_ctx.path_type = typePath; // duplicated
                    task.success(msg);
                } else {
                    std::string msg = "options errors in get crit path list telegram: " + options.errorsStr();
                    std::cerr << msg << std::endl;
                    task.fail(msg);
                }
            } else if (task.cmd() == CMD_DRAW_PATH_ID) {
                options.validateNamePresence({OPTION_PATH_INDEX, OPTION_HIGHTLIGHT_MODE});
                int pathIndex = options.getInt(OPTION_PATH_INDEX, -1);
                std::string highLightMode = options.getString(OPTION_HIGHTLIGHT_MODE);
                if (!options.hasErrors()) {
                    if ((pathIndex >= 0) && (pathIndex < g_vpr_ctx.crit_paths.size())) {
                        g_vpr_ctx.crit_path_index = pathIndex;

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
                        std::string msg = "selectedIndex=" + std::to_string(pathIndex) + " is out of range [0-" + std::to_string(g_vpr_ctx.crit_paths.size()-1) + "]";
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
}
#else
void TaskResolver::update()
{
    for (auto& task: m_tasks) {
        if (!task.isFinished()) {
#ifdef ENABLE_TASK_DELAY
            std::this_thread::sleep_for(std::chrono::milliseconds(TASK_RESOLVE_DELAY_MS));
#endif
            task.success(generateRandomPathsString());
        }
    }
}
#endif
