#ifndef VPR_EXIT_CODES_H
#define VPR_EXIT_CODES_H

/*
 * Exit codes to signal success/failure to scripts
 * calling vpr
 */
constexpr int SUCCESS_EXIT_CODE = 0;         //Everything OK
constexpr int ERROR_EXIT_CODE = 1;           //Something went wrong internally
constexpr int UNIMPLEMENTABLE_EXIT_CODE = 2; //Could not implement (e.g. unroutable)
constexpr int INTERRUPTED_EXIT_CODE = 3;     //VPR was interrupted by the user (e.g. SIGINT/ctr-C)

#endif
