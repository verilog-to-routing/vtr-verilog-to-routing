#include "odin_ii.h"
#include "odin_types.h"
#include "vtr_time.h"
#include "vtr_version.h"
#include <sys/resource.h>

int main(int argc, char** argv) {
    vtr::ScopedFinishTimer t("Odin II");

    printf(
        "=======================================================================\n"
        " Odin II - Verilog synthesis tools targetting VPR FPGAs\n"
        "-----------------------------------------------------------------------\n"
        "\tVersion: %s\n"
        "\tRevision: %s\n"
        "\tCompiled: %s\n"
        "\tCompiler: %s\n"
        "\tBuild Info: %s\n"
        "\n"
        "University of New Brunswick\n"
        "For documentation:\n"
        "  https://docs.verilogtorouting.org/en/latest/odin\n"
        "For question:\n"
        "  vtr-users@googlegroups.com\n"
        "\n"
        "This is free open source code under MIT license.\n"
        "=======================================================================\n"
        "\n",
        vtr::VERSION, vtr::VCS_REVISION, vtr::BUILD_TIMESTAMP, vtr::COMPILER, vtr::BUILD_INFO);

/**
 * set stack size to >= 128 MB in linux operating systems to avoid
 * stack overflow in recursive routine calls for big benchmarks 
 */
#ifdef __linux__
    struct rlimit rl;
    const rlim_t new_stack_size = 128L * 1024L * 1024L;
    if (getrlimit(RLIMIT_STACK, &rl) == 0 && rl.rlim_cur < new_stack_size) {
        rl.rlim_cur = new_stack_size;
        setrlimit(RLIMIT_STACK, &rl);
    }
#endif

    netlist_t* odin_netlist = start_odin_ii(argc, argv);
    terminate_odin_ii(odin_netlist);
    return 0;
}