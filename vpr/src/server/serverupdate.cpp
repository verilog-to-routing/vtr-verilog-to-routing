#include "serverupdate.h"
#include "gateio.h"
#include "taskresolver.h"
#include "globals.h"
#include "ezgl/application.hpp"

#ifndef NO_GRAPHICS

namespace server {

gboolean update(gpointer data) {
    bool isRunning = g_vpr_ctx.server().gateIO().isRunning();
    if (isRunning) {
        // shortcuts
        ezgl::application* app = static_cast<ezgl::application*>(data);
        GateIO& gate_io = g_vpr_ctx.mutable_server().mutable_gateIO();
        TaskResolver& task_resolver = g_vpr_ctx.mutable_server().mutable_task_resolver();

        std::vector<TaskPtr> tasksBuff;

        gate_io.takeRecievedTasks(tasksBuff);
        task_resolver.addTasks(tasksBuff);

        bool process_task = task_resolver.update(app);

        tasksBuff.clear();
        task_resolver.takeFinished(tasksBuff);

        gate_io.moveTasksToSendQueue(tasksBuff);
        gate_io.printLogs();

        // Call the redraw method of the application if any of task was processed
        if (process_task) {
            app->refresh_drawing();
        }
    }
    
    // Return TRUE to keep the timer running, or FALSE to stop it
    return isRunning;
}

} // namespace server

#endif // NO_GRAPHICS
