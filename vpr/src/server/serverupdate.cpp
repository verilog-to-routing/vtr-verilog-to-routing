#ifndef NO_SERVER

#include "serverupdate.h"
#include "gateio.h"
#include "taskresolver.h"
#include "globals.h"
#include "ezgl/application.hpp"

namespace server {

gboolean update(gpointer data) {
    bool is_running = g_vpr_ctx.server().gateIO().isRunning();
    if (is_running) {
        // shortcuts
        ezgl::application* app = static_cast<ezgl::application*>(data);
        GateIO& gate_io = g_vpr_ctx.mutable_server().mutable_gateIO();
        TaskResolver& task_resolver = g_vpr_ctx.mutable_server().mutable_task_resolver();

        std::vector<TaskPtr> tasks_buff;

        gate_io.takeReceivedTasks(tasks_buff);
        for (TaskPtr& task: tasks_buff) {
            task_resolver.ownTask(std::move(task));
        }
        tasks_buff.clear();

        bool has_finished_tasks = task_resolver.update(app);

        task_resolver.takeFinished(tasks_buff);

        gate_io.moveTasksToSendQueue(tasks_buff);
        gate_io.printLogs();

        // Call the redraw method of the application if any of task was processed
        if (has_finished_tasks) {
            app->refresh_drawing();
        }
    }
    
    // Return TRUE to keep the timer running, or FALSE to stop it
    return is_running;
}

} // namespace server

#endif // NO_SERVER
