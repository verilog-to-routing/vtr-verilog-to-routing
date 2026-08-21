#include "constraints_load.h"
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
    vtr::ScopedStartFinishTimer timer("Reading VPR constraints from " + std::string(read_vpr_constraints_name));

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
    floorplanning_ctx.constraints = reader.constraints_.place_constraints();
    floorplanning_ctx.relative_macros = reader.constraints_.relative_macros();

    const UserRelativeMacros& relative_macros = floorplanning_ctx.relative_macros;
    if (relative_macros.get_num_macros() > 0) {
        VTR_LOG("Read %zu relative placement macro(s):\n", relative_macros.get_num_macros());
        for (size_t imacro = 0; imacro < relative_macros.get_num_macros(); imacro++) {
            const UserRelativeMacro& macro = relative_macros.get_macro(UserRelativeMacroId(imacro));
            size_t num_atoms = 0;
            for (const UserRelativeGroup& group : macro.groups) {
                num_atoms += group.atoms.size();
            }
            VTR_LOG("  Relative macro '%s': %zu group(s), %zu atom(s)\n",
                    macro.name.c_str(), macro.groups.size(), num_atoms);
        }
    }

    auto& routing_ctx = g_vpr_ctx.mutable_routing();
    routing_ctx.constraints = reader.constraints_.route_constraints();

    const auto& ctx_constraints = floorplanning_ctx.constraints;

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_VPR_CONSTRAINTS)) {
        echo_constraints(getEchoFileName(E_ECHO_VPR_CONSTRAINTS), ctx_constraints);
    }
}
