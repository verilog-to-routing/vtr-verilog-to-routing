#ifndef SERVERUPDATE_H
#define SERVERUPDATE_H

#ifndef NO_SERVER

#include <glib.h>

namespace server {

/**
 * @brief Main server update callback.
 * 
 * This function is a periodic callback invoked at a fixed interval to manage and handle incoming client requests.
 * It acts as the central control point for processing client interactions and orchestrating server-side operations 
 * within the specified time intervals.
 */
gboolean update(gpointer);

} // namespace server

#endif /* NO_SERVER */

#endif /* SERVERUPDATE_H */

