#include "taskresolver.h"
#ifdef ENABLE_RANDOM_PATH_GENERATION
#include <server_app/pathsgen.h>
#endif

#ifndef STANDALONE_APP
#include "globals.h"
#include "pathhelper.h"
#include "telegramparser.h"
#include <gtk/gtk.h>
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
                task.error(msg);
            } else {
                if (task.jobId() > t.jobId()) {
                    std::string msg = "old task: " + t.info() + " is overriden by a new task: " + task.info();
                    t.error(msg);
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
            // ugly, but let's have it for now
            std::vector<std::string> optionsList = splitString(task.options(), ';');
            if (task.cmd() == CMD_GET_PATH_LIST_ID) {
                if (optionsList.size() == 4) {
                    int nCriticalPathNum = std::atoi(optionsList[0].c_str());
                    std::string typePath = optionsList[1];
                    int detailsLevel = std::atoi(optionsList[2].c_str());
                    bool isFlat = std::atoi(optionsList[3].c_str());

                    calcCritPath(nCriticalPathNum, typePath);
                    std::string msg = getPathsStr(g_vpr_ctx.crit_paths, detailsLevel, isFlat);
                    if (typePath != g_vpr_ctx.path_type) {
                        g_vpr_ctx.crit_path_index = -1;
                    }
                    g_vpr_ctx.path_type = typePath; // duplicated
                    task.success(msg);
                } else {
                    std::string msg = "bad options [" + task.options() + "] for get crit path list telegram";
                    std::cerr << msg << std::endl;
                    task.error(msg);
                }
            } else if (task.cmd() == CMD_DRAW_PATH_ID) {
                if (optionsList.size() == 2) {
                    int pathIndex = telegramparser::extractPathIndex(optionsList[0]) - 1;
                    int highLightMode = 1;
                    
                    try {
                        highLightMode = std::atoi(optionsList[1].c_str());
                    } catch(...) {

                    }

                    if ((pathIndex >= 0) && (pathIndex < g_vpr_ctx.crit_paths.size())) {
                        g_vpr_ctx.crit_path_index = pathIndex;

                        // update gtk UI
                        GtkComboBox* toggle_crit_path = GTK_COMBO_BOX(app->get_object("ToggleCritPath"));
                        gtk_combo_box_set_active(toggle_crit_path, highLightMode);

                        task.success();
                    } else {
                        std::string msg = "selectedIndex=" + std::to_string(pathIndex) + " is out of range [0-" + std::to_string(g_vpr_ctx.crit_paths.size()-1) + "]";
                        task.error(msg);
                    }
                } else {
                    std::string msg = "bad options [" + task.options() + "] for highlight crit path telegram";
                    std::cerr << msg << std::endl;
                    task.error(msg);
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
