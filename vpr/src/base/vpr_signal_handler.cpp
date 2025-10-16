/**
 * @file
 * @brief This file defines a signal handler used by VPR to catch SIGINT signals
 *        (ctrl-C from terminal) on POSIX systems. It is only active if
 *        VPR_USE_SIGACTION is defined.
 *
 * Behavior:
 *  - SIGINT  : log, attempt to checkpoint, then exit with INTERRUPTED_EXIT_CODE
 *  - SIGHUP  : log, attempt to checkpoint, continue running
 *  - SIGTERM : log, attempt to checkpoint, then exit with INTERRUPTED_EXIT_CODE
 */
#include "vtr_time.h"

#include "vpr_signal_handler.h"
#include "globals.h"

#include "read_place.h"
#include "read_route.h"

// Currenly safe_write uses the POSIX write system call. This could be extended to other platforms in the future.
#if defined(__unix__)
#include "unistd.h"
#endif

#include "string.h"

#ifdef VPR_USE_SIGACTION
#include <csignal>
#endif

void vpr_signal_handler(int signal);
void checkpoint();

/**
 * @brief Writes a message directly to stderr with async-signal-safe write() function.
 *
 * Uses write() to avoid signal unsafe std::cerr in the signal handler.
 *
 * @param msg Message string to write.
 */
static inline void safe_write(const char* msg) {
    (void)!write(STDERR_FILENO, msg, strlen(msg));
}

#ifdef VPR_USE_SIGACTION

void vpr_install_signal_handler() {
    //Install the signal handler for SIGINT (i.e. ctrl+C from the terminal)
    struct sigaction sa;
    sa.sa_handler = vpr_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;                  //Make sure the flags are cleared
    sigaction(SIGINT, &sa, nullptr);  //Keyboard interrupt (1 pauses, 2 quits)
    sigaction(SIGHUP, &sa, nullptr);  //Hangup (attempts to checkpoint)
    sigaction(SIGTERM, &sa, nullptr); //Terminate (attempts to checkpoint, then exit)
}

void vpr_signal_handler(int signal) {
    if (signal == SIGINT) {
        safe_write("Received SIGINT: Attempting to checkpoint then exit...\n");
        checkpoint();
        std::quick_exit(INTERRUPTED_EXIT_CODE);
    } else if (signal == SIGHUP) {
        safe_write("Received SIGHUP: Attempting to checkpoint...\n");
        checkpoint();
    } else if (signal == SIGTERM) {
        safe_write("Received SIGTERM: Attempting to checkpoint then exit...\n");
        checkpoint();
        std::quick_exit(INTERRUPTED_EXIT_CODE);
    }
}

#else
//No signal support all operations are no-ops

void vpr_install_signal_handler() {
    //Do nothing
}

void vpr_signal_handler(int /*signal*/) {
    //Do nothing
}

#endif

void checkpoint() {
    //Dump the current placement and routing state
    vtr::ScopedStartFinishTimer timer("Checkpointing");

    safe_write("Attempting to checkpoint current placement to file: placer_checkpoint.place\n");
    print_place(nullptr, nullptr, "placer_checkpoint.place", g_vpr_ctx.placement().block_locs());

    bool is_flat = g_vpr_ctx.routing().is_flat;
    const Netlist<>& router_net_list = is_flat ? (const Netlist<>&)g_vpr_ctx.atom().netlist() : (const Netlist<>&)g_vpr_ctx.clustering().clb_nlist;

    safe_write("Attempting to checkpoint current routing to file: router_checkpoint.route\n");
    print_route(router_net_list, nullptr, "router_checkpoint.route", is_flat);
}
