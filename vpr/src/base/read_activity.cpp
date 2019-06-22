#include <cstring>

#include "read_activity.h"

#include "vtr_log.h"
#include "vtr_util.h"

#include "vpr_error.h"

#include "atom_netlist.h"

static bool add_activity_to_net(const AtomNetlist& netlist, std::unordered_map<AtomNetId, t_net_power>& atom_net_power, char* net_name, float probability, float density);

static bool add_activity_to_net(const AtomNetlist& netlist, std::unordered_map<AtomNetId, t_net_power>& atom_net_power, char* net_name, float probability, float density) {
    AtomNetId net_id = netlist.find_net(net_name);
    if (net_id) {
        atom_net_power[net_id].probability = probability;
        atom_net_power[net_id].density = density;
        return false;
    }

    VTR_LOG(
        "Error: net %s found in activity file, but it does not exist in the .blif file.\n",
        net_name);
    return true;
}

std::unordered_map<AtomNetId, t_net_power> read_activity(const AtomNetlist& netlist, const char* activity_file) {
    char buf[vtr::bufsize];
    char* ptr;
    char* word1;
    char* word2;
    char* word3;

    FILE* act_file_hdl;

    std::unordered_map<AtomNetId, t_net_power> atom_net_power;

    for (auto net_id : netlist.nets()) {
        atom_net_power[net_id].probability = -1.0;
        atom_net_power[net_id].density = -1.0;
    }

    act_file_hdl = vtr::fopen(activity_file, "r");
    if (act_file_hdl == nullptr) {
        vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
                  "Error: could not open activity file: %s\n", activity_file);
    }

    ptr = vtr::fgets(buf, vtr::bufsize, act_file_hdl);
    while (ptr != nullptr) {
        word1 = strtok(buf, TOKENS);
        word2 = strtok(nullptr, TOKENS);
        word3 = strtok(nullptr, TOKENS);
        add_activity_to_net(netlist, atom_net_power, word1, vtr::atof(word2), vtr::atof(word3));

        ptr = vtr::fgets(buf, vtr::bufsize, act_file_hdl);
    }
    fclose(act_file_hdl);

    /* Make sure all nets have an activity value */
    for (auto net_id : netlist.nets()) {
        if (atom_net_power[net_id].probability < 0.0
            || atom_net_power[net_id].density < 0.0) {
            vpr_throw(VPR_ERROR_BLIF_F, __FILE__, __LINE__,
                      "Error: Activity file does not contain signal %s\n",
                      netlist.net_name(net_id).c_str());
        }
    }
    return atom_net_power;
}
