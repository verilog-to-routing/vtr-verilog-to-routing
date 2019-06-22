/*
 * This file defines a signal handler used by VPR to catch SIGINT signals
 * (ctrl-C from terminal) on POSIX systems. It is only active if
 * VPR_USE_SIGACTION is defined.
 *
 * If a SIGINT occur the handler sets the 'forced_pause' flag of the VPR
 * context.  If 'forced_pause' is still true when another SIGINT occurs an
 * exception is thrown (typically ending the program).
 */
#include "vtr_log.h"
#include "vtr_time.h"

#include "vpr_signal_handler.h"
#include "vpr_exit_codes.h"
#include "vpr_error.h"
#include "globals.h"

#include "read_place.h"
#include "route_export.h"
#include <atomic>

#ifdef VPR_USE_SIGACTION
#    include <csignal>
#endif

void vpr_signal_handler(int signal);
void checkpoint();

std::atomic<int> uncleared_sigint_count(0);

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
        if (g_vpr_ctx.forced_pause()) {
            uncleared_sigint_count++; //Previous SIGINT uncleared
        } else {
            uncleared_sigint_count.store(1); //Only this SIGINT outstanding
        }

        if (uncleared_sigint_count == 1) {
            //Request a pause at the next reasonable point (e.g. to resume the GUI)
            VTR_LOG("Recieved SIGINT: Attempting to pause...\n");
            g_vpr_ctx.set_forced_pause(true);
        } else if (uncleared_sigint_count == 2) {
            VTR_LOG("Recieved two uncleared SIGINTs: Attempting to checkpoint and exit...\n");
            checkpoint();
            std::quick_exit(INTERRUPTED_EXIT_CODE);
        } else if (uncleared_sigint_count == 3) {
            //Really exit (e.g. SIGINT while checkpointing)
            VTR_LOG("Recieved three uncleared SIGINTs: Exiting...\n");
            std::quick_exit(INTERRUPTED_EXIT_CODE);
        }
    } else if (signal == SIGHUP) {
        VTR_LOG("Recieved SIGHUP: Attempting to checkpoint...\n");
        checkpoint();
    } else if (signal == SIGTERM) {
        VTR_LOG("Recieved SIGTERM: Attempting to checkpoint then exit...\n");
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

    std::string placer_checkpoint_file = "placer_checkpoint.place";
    VTR_LOG("Attempting to checkpoint current placement to file: %s\n", placer_checkpoint_file.c_str());
    print_place(nullptr, nullptr, placer_checkpoint_file.c_str());

    std::string router_checkpoint_file = "router_checkpoint.route";
    VTR_LOG("Attempting to checkpoint current routing to file: %s\n", router_checkpoint_file.c_str());
    print_route(nullptr, router_checkpoint_file.c_str());
}
