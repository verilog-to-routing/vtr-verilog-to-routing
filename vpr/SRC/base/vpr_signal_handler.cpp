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

#include "vpr_signal_handler.h"
#include "vpr_error.h"
#include "globals.h"

#ifdef VPR_USE_SIGACTION
#include <csignal>
#endif

void vpr_signal_handler(int signal);

#ifdef VPR_USE_SIGACTION

    void vpr_install_signal_handler() {
        //Install the signal handler for SIGINT (i.e. ctrl+C from the terminal)
        struct sigaction sa;
        sa.sa_handler = vpr_signal_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0; //Make sure the flags are cleared
        sigaction(SIGINT, &sa, nullptr);
    }

    void vpr_signal_handler(int signal) {
        VTR_ASSERT_MSG(signal == SIGINT, "Expected to handle only SIGINT");

        bool already_paused = g_vpr_ctx.forced_pause();

        if (!already_paused) {
            //Ask VPR to pause at the next conveneient point
            g_vpr_ctx.set_forced_pause(true);
        } else {
            //Really stop
            VPR_THROW(VPR_ERROR_INTERRUPTED, "Interrupted by user");
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
