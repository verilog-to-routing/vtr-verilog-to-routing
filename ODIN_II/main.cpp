#include "odin_ii.h"
#include "odin_types.h"
#include "vtr_time.h"
#include "vtr_version.h"

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

    netlist_t* odin_netlist = start_odin_ii(argc, argv);
    terminate_odin_ii(odin_netlist);
    return 0;
}