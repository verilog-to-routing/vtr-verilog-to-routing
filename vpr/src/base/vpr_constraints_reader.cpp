#include "vpr_constraints_serializer.h"
#include "vpr_constraints_uxsdcxx.h"

#include "vtr_time.h"

#include "globals.h"
#include "pugixml.hpp"
#include "pugixml_util.hpp"
#include "echo_files.h"

#include <fstream>
#include "vpr_constraints_reader.h"

void load_vpr_constraints_file(const char* read_vpr_constraints_name) {
    vtr::ScopedStartFinishTimer timer("Loading VPR constraints file");

    VprConstraintsSerializer reader;

    if (vtr::check_file_name_extension(read_vpr_constraints_name, ".xml")) {
        try {
            std::ifstream file(read_vpr_constraints_name);
            void* context;
            uxsd::load_vpr_constraints_xml(reader, context, read_vpr_constraints_name, file);
        } catch (pugiutil::XmlError& e) {
            vpr_throw(VPR_ERROR_ROUTE, read_vpr_constraints_name, e.line(), "%s", e.what());
        }
    } else {
        VTR_LOG_WARN(
            "VPR constraints file '%s' may be in incorrect format. "
            "Expecting .xml format. Not reading file.\n",
            read_vpr_constraints_name);
    }

    //Update the floorplanning constraints in the floorplanning constraints context
    auto& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();
    floorplanning_ctx.constraints = reader.constraints_;

    VprConstraints ctx_constraints = floorplanning_ctx.constraints;

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_VPR_CONSTRAINTS)) {
        echo_constraints(getEchoFileName(E_ECHO_VPR_CONSTRAINTS), ctx_constraints);
    }
}
