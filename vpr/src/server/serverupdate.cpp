#ifndef NO_SERVER

#include "serverupdate.h"
#include "gateio.h"
#include "taskresolver.h"
#include "globals.h"
#include "ezgl/application.hpp"

namespace server {

gboolean update(gpointer data) {
    const bool is_running = g_vpr_ctx.server().gate_io.is_running();
    if (is_running) {
        // shortcuts
        ezgl::application* app = static_cast<ezgl::application*>(data);
        GateIO& gate_io = g_vpr_ctx.mutable_server().gate_io;
        TaskResolver& task_resolver = g_vpr_ctx.mutable_server().task_resolver;

        std::vector<TaskPtr> tasks_buff;

        gate_io.take_received_tasks(tasks_buff);
        for (TaskPtr& task: tasks_buff) {
            task_resolver.own_task(std::move(task));
        }
        tasks_buff.clear();

        const bool is_server_context_initialized = g_vpr_ctx.server().timing_info && g_vpr_ctx.server().routing_delay_calc;
        if (is_server_context_initialized) {
            bool has_finished_tasks = task_resolver.update(app);

            task_resolver.take_finished_tasks(tasks_buff);

            gate_io.move_tasks_to_send_queue(tasks_buff);

            // Call the redraw method of the application if any of task was processed
            if (has_finished_tasks) {
                app->refresh_drawing();
            }
        }
        gate_io.print_logs();
    }
    
    // Return TRUE to keep the timer running, or FALSE to stop it
    return is_running;
}

} // namespace server

#endif // NO_SERVER
