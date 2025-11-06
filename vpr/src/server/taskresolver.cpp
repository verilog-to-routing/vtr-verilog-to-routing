#ifndef NO_SERVER

#include "taskresolver.h"

#include "commconstants.h"
#include "globals.h"
#include "pathhelper.h"
#include "telegramoptions.h"
#include <gtk/gtk.h>


#include <ezgl/application.hpp>

namespace server {

void TaskResolver::own_task(TaskPtr&& new_task) {
    // pre-process task before adding, where we could quickly detect failure scenarios
    for (const auto& task : m_tasks) {
        if (task->cmd() == new_task->cmd()) {
            if (task->options_match(new_task)) {
                std::string msg = "similar task is already in execution, reject new " + new_task->info() + " and waiting for old " + task->info() + " execution";
                new_task->set_fail(msg);
            } else {
                // handle case when task has same cmd but different options
                if (new_task->job_id() > task->job_id()) {
                    std::string msg = "old " + task->info() + " is overridden by a new " + new_task->info();
                    task->set_fail(msg);
                }
            }
        }
    }

    // own task
    m_tasks.emplace_back(std::move(new_task));
}

void TaskResolver::take_finished_tasks(std::vector<TaskPtr>& result) {
    for (auto it = m_tasks.begin(); it != m_tasks.end();) {
        TaskPtr& task = *it;
        if (task->is_finished()) {
            result.push_back(std::move(task));
            it = m_tasks.erase(it);
        } else {
            ++it;
        }
    }
}

std::optional<e_timing_report_detail> TaskResolver::try_get_details_level_enum(const std::string& path_details_level_str) const {
    if (path_details_level_str == "netlist") {
        return e_timing_report_detail::NETLIST;
    } else if (path_details_level_str == "aggregated") {
        return e_timing_report_detail::AGGREGATED;
    } else if (path_details_level_str == "detailed") {
        return e_timing_report_detail::DETAILED_ROUTING;
    } else if (path_details_level_str == "debug") {
        return e_timing_report_detail::DEBUG;
    }

    return std::nullopt;
}

bool TaskResolver::update(ezgl::application* app) {
    bool has_processed_task = false;
    for (auto& task : m_tasks) {
        if (!task->is_finished()) {
            switch (task->cmd()) {
                case comm::CMD::GET_PATH_LIST_ID: {
                    process_get_path_list_task(app, task);
                    has_processed_task = true;
                    break;
                }
                case comm::CMD::DRAW_PATH_ID: {
                    process_draw_critical_path_task(app, task);
                    has_processed_task = true;
                    break;
                }
                default:
                    break;
            }
        }
    }

    return has_processed_task;
}

void TaskResolver::process_get_path_list_task(ezgl::application*, const TaskPtr& task) {
    static const std::vector<std::string> keys{comm::OPTION_PATH_NUM, comm::OPTION_PATH_TYPE, comm::OPTION_DETAILS_LEVEL, comm::OPTION_IS_FLAT_ROUTING};
    TelegramOptions options{task->options(), keys};
    if (!options.has_errors()) {
        ServerContext& server_ctx = g_vpr_ctx.mutable_server(); // shortcut

        server_ctx.crit_path_element_indexes.clear(); // reset selection if path list options has changed

        // read options
        const int n_critical_path_num = options.get_int(comm::OPTION_PATH_NUM, 1);
        const std::string path_type = options.get_string(comm::OPTION_PATH_TYPE);
        const std::string details_level_str = options.get_string(comm::OPTION_DETAILS_LEVEL);
        const bool is_flat = options.get_bool(comm::OPTION_IS_FLAT_ROUTING, false);

        // calculate critical path depending on options and store result in server context
        std::optional<e_timing_report_detail> details_level_opt = try_get_details_level_enum(details_level_str);
        if (details_level_opt) {
            CritPathsResultPtr crit_paths_result = calc_critical_path(path_type, n_critical_path_num, details_level_opt.value(), is_flat);
            if (crit_paths_result->is_valid()) {
                server_ctx.crit_paths = std::move(crit_paths_result->paths);
                task->set_success(std::move(crit_paths_result->report));
            } else {
                std::string msg{"Critical paths report is empty"};
                VTR_LOG_ERROR(msg.c_str());
                task->set_fail(msg);
            }
        } else {
            std::string msg{"unsupported report details level " + details_level_str};
            VTR_LOG_ERROR(msg.c_str());
            task->set_fail(msg);
        }
    } else {
        std::string msg{"options errors in get crit path list telegram: " + options.errors_str()};
        VTR_LOG_ERROR(msg.c_str());
        task->set_fail(msg);
    }
}

void TaskResolver::process_draw_critical_path_task(ezgl::application* app, const TaskPtr& task) {
    TelegramOptions options{task->options(), {comm::OPTION_PATH_ELEMENTS, comm::OPTION_HIGHLIGHT_MODE, comm::OPTION_DRAW_PATH_CONTOUR}};
    if (!options.has_errors()) {
        ServerContext& server_ctx = g_vpr_ctx.mutable_server(); // shortcut

        const std::map<std::size_t, std::set<std::size_t>> path_elements = options.get_map_of_sets(comm::OPTION_PATH_ELEMENTS);
        const std::string high_light_mode = options.get_string(comm::OPTION_HIGHLIGHT_MODE);
        const bool draw_path_contour = options.get_bool(comm::OPTION_DRAW_PATH_CONTOUR, false);

        // set critical path elements to render
        server_ctx.crit_path_element_indexes = std::move(path_elements);
        server_ctx.draw_crit_path_contour = draw_path_contour;

        // get GTK widgets
        GtkSwitch* crit_path_switch = GTK_SWITCH(app->get_object("ToggleCritPath"));
        GtkToggleButton* crit_path_flylines_button = GTK_TOGGLE_BUTTON(app->get_object("ToggleCritPathFlylines"));
        GtkToggleButton* crit_path_routing_button = GTK_TOGGLE_BUTTON(app->get_object("ToggleCritPathRouting"));
        GtkToggleButton* crit_path_delays_button = GTK_TOGGLE_BUTTON(app->get_object("ToggleCritPathDelays"));

        if (crit_path_switch && crit_path_flylines_button && crit_path_routing_button && crit_path_delays_button) {
            bool draw_flylines = (high_light_mode.find("flylines") != std::string::npos);
            bool draw_routing = (high_light_mode.find("routing") != std::string::npos);
            bool draw_delays = (high_light_mode.find("delays") != std::string::npos);

            gtk_switch_set_active(crit_path_switch, TRUE);
            gtk_toggle_button_set_active(crit_path_flylines_button, draw_flylines);
            gtk_toggle_button_set_active(crit_path_routing_button, draw_routing);
            gtk_toggle_button_set_active(crit_path_delays_button, draw_delays);
            task->set_success();
        } else {
            std::string msg{"Cannot find critical path widgets. Was the UI layout changed?"};
            VTR_LOG_ERROR(msg.c_str());
            task->set_fail(msg);
        }
    } else {
        std::string msg{"options errors in highlight crit path telegram: " + options.errors_str()};
        VTR_LOG_ERROR(msg.c_str());
        task->set_fail(msg);
    }
}

} // namespace server

#endif /* NO_SERVER */
