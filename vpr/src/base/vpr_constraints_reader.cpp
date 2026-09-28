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

    // A design can carry many macros, and the per-macro
    // detail is written to the vpr_constraints echo file
    const UserRelativeMacros& relative_macros = floorplanning_ctx.relative_macros;
    if (relative_macros.get_num_macros() > 0) {
        size_t num_groups = 0;
        size_t num_atoms = 0;
        size_t num_locked_atoms = 0;
        for (UserRelativeMacroId macro_id : relative_macros.macros()) {
            const t_user_relative_macro& macro = relative_macros.get_macro(macro_id);
            num_groups += macro.groups.size();
            for (const t_user_relative_group& group : macro.groups) {
                num_atoms += group.atoms.size();
                for (const std::string& site_path : group.atom_site_paths) {
                    if (!site_path.empty())
                        num_locked_atoms++;
                }
            }
        }
        VTR_LOG("Read %zu relative placement macro(s): %zu group(s), %zu atom(s), %zu atom(s) locked to a primitive site\n",
                relative_macros.get_num_macros(), num_groups, num_atoms, num_locked_atoms);
    }

    auto& routing_ctx = g_vpr_ctx.mutable_routing();
    routing_ctx.constraints = reader.constraints_.route_constraints();

    const auto& ctx_constraints = floorplanning_ctx.constraints;

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_VPR_CONSTRAINTS)) {
        echo_constraints(getEchoFileName(E_ECHO_VPR_CONSTRAINTS), ctx_constraints, relative_macros);
    }

    // Temporary guard: the packer and placer do not honor relative placement
    // macros yet. Stop here rather than silently ignore the constraints.
    // Remove once packing and placement support for relative macros is in.
    if (relative_macros.get_num_macros() > 0) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Constraints file '%s' contains relative placement macros, which are not yet honored by the flow. "
                        "Remove the <relative_macro_list> to run without them.\n",
                        read_vpr_constraints_name);
    }
}
