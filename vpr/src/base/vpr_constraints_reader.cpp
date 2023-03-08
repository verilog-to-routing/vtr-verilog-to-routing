#include "vpr_constraints_serializer.h"
#include "vpr_constraints_uxsdcxx.h"

#include "vtr_time.h"

#include "globals.h"
#include "pugixml.hpp"
#include "pugixml_util.hpp"
#include "echo_files.h"

#include <fstream>
#include "vpr_constraints_reader.h"
#include "vtr_token.h"

void load_vpr_constraints_file(const char* read_vpr_constraints_name) {
    vtr::ScopedStartFinishTimer timer("Reading VPR constraints from " + std::string(read_vpr_constraints_name));

    VprConstraintsSerializer reader;

    // file name from arguments could be a serial of files, seperated by colon ":"
    // at this point, caller has already checked that the required constraint file name is not emtpy
    int num_tokens = 0, num_file_read = 0;
    bool found_file = false;
    t_token* tokens = GetTokensFromString(read_vpr_constraints_name, &num_tokens);
    std::string in_tokens("");
    for (int i = 0; i < num_tokens; i++) {
        if ((tokens[i].type == TOKEN_COLON)) { // end of one input file
            found_file = true;
        } else if (i == num_tokens - 1) { // end of inputs, append token anyway
            in_tokens += std::string(tokens[i].data);
            found_file = true;
        } else {
            in_tokens += std::string(tokens[i].data);
        }
        if (found_file) {
            const char* file_name = in_tokens.c_str();
            if (vtr::check_file_name_extension(file_name, ".xml")) {
                try {
                    std::ifstream file(file_name);
                    void* context;
                    uxsd::load_vpr_constraints_xml(reader, context, file_name, file);
                } catch (pugiutil::XmlError& e) {
                    vpr_throw(VPR_ERROR_ROUTE, file_name, e.line(), "%s", e.what());
                }
            } else {
                VTR_LOG_WARN(
                    "VPR constraints file '%s' may be in incorrect format. "
                    "Expecting .xml format. Not reading file.\n",
                    file_name);
            }
            in_tokens.clear();
            num_file_read++;
            found_file = false;
        }
    }
    VTR_LOG("Read in '%d' constraint file(s) successfully.\n", num_file_read);
    freeTokens(tokens, num_tokens);

    //Update the floorplanning constraints in the floorplanning constraints context
    auto& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();
    floorplanning_ctx.constraints = reader.constraints_.place_constraints();

    auto& routing_ctx = g_vpr_ctx.mutable_routing();
    routing_ctx.constraints = reader.constraints_.route_constraints();

    // update vpr constraints for routing
    auto& routing_ctx = g_vpr_ctx.mutable_routing();
    routing_ctx.constraints = reader.constraints_;

    VprConstraints ctx_constraints = floorplanning_ctx.constraints;

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_VPR_CONSTRAINTS)) {
        echo_constraints(getEchoFileName(E_ECHO_VPR_CONSTRAINTS), ctx_constraints);
    }
}
