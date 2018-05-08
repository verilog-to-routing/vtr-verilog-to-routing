#include "molecule_dfa.h"

#include "atom_netlist.h"
#include "vpr_utils.h"

int MoleculeDFA::add_state(bool accepting) {
    int id = states_.size();
    states_.emplace_back(accepting);
    return id;
}

int MoleculeDFA::add_state_transition(int from_state, int to_state, t_netlist_pack_pattern_edge pattern_edge) {
    StateTransition transition;
    transition.pack_pattern_edge = pattern_edge;
    transition.from_state = from_state;
    transition.next_state = to_state;

    int id = transitions_.size();
    transitions_.push_back(transition);

    states_[from_state].transitions.push_back(id);

    return id;
}

void MoleculeDFA::set_start_state(int state) {
    start_state_ = state;
}

std::set<MoleculeDFA::Match> MoleculeDFA::match_all(const AtomNetlist& netlist) const {
    std::vector<std::set<Match>> current_matches(states_.size());
    std::vector<std::set<Match>> next_matches(states_.size());

    //Initially, every block is in the start state
    for (AtomBlockId blk : netlist.blocks()) {
        next_matches[start_state_].emplace(blk);
    }

    //Run the DFA
    std::set<Match> accepted_matches;
    while (current_matches != next_matches) { //State changed
        current_matches = next_matches; //Move to next match state
        collect_accepted(accepted_matches, current_matches); //Record full matches
        next_matches = dfa_step(current_matches, netlist); //Calculate next match state
    }

    return accepted_matches;
}

std::vector<std::set<MoleculeDFA::Match>> MoleculeDFA::dfa_step(const std::vector<std::set<Match>>& current_matches, const AtomNetlist& netlist) const {
    std::vector<std::set<Match>> next_matches(states_.size());

    //Update each state
    for (size_t istate = 0; istate < states_.size(); ++istate) {

        //Update each match
        for (const Match& curr_match : current_matches[istate]) {

            //Evaluate each potential transition out of the current state
            for (size_t itrans = 0; itrans < states_[istate].transitions.size(); ++itrans) {
                
                Match new_match;
                if (matches(new_match, curr_match, transitions_[itrans].pack_pattern_edge, netlist)) {
                    next_matches[transitions_[itrans].next_state].insert(new_match);
                }
            }
        }
    }

    return next_matches;
}

void MoleculeDFA::collect_accepted(std::set<Match>& accepted_matches, const std::vector<std::set<Match>>& current_matches) const {
    for (int istate = 0; istate < (int) states_.size(); ++istate) {
        if (states_[istate].accepting) {
            accepted_matches.insert(current_matches[istate].begin(), current_matches[istate].end());
        }
    }
}

bool MoleculeDFA::matches(Match& new_match, const Match& old_match, const t_netlist_pack_pattern_edge& pattern_edge, const AtomNetlist& netlist) const {
    
    //Collect blocks from the existing match
    std::set<AtomBlockId> blocks = {old_match.root};
    for (auto& edge : old_match.netlist_edges) {
        
        if (edge.from_pin) {
            blocks.insert(netlist.pin_block(edge.from_pin));
        }
        if (edge.to_pin) {
            blocks.insert(netlist.pin_block(edge.to_pin));
        }
    }

    //Check if any of the blocks in the current match can be extended by pattern_edge
    for (auto blk : blocks) {
        AtomPinId from_pin;
        AtomPinId to_pin;
        if (block_matches(from_pin, to_pin, blk, pattern_edge, netlist)) {
            //Copy old match
            new_match = old_match;

            //Add new edge
            new_match.netlist_edges.emplace_back(from_pin, to_pin);

            return true; //Successfully extended match
        }
    }

    return false; //No match found
}

bool MoleculeDFA::block_matches(AtomPinId& from_pin, AtomPinId& to_pin, const AtomBlockId from_blk, 
                                const t_netlist_pack_pattern_edge& pattern_edge, const AtomNetlist& netlist) const {
    t_model* from_blk_model = pattern_edge.from_model;
    t_model* to_blk_model = pattern_edge.to_model;
    VTR_ASSERT(from_blk_model);
    VTR_ASSERT(to_blk_model);

    BitIndex from_port_pin = pattern_edge.from_port_pin;
    BitIndex to_port_pin = pattern_edge.to_port_pin;

    t_model_ports* from_model_port = pattern_edge.from_model_port;
    t_model_ports* to_model_port = pattern_edge.to_model_port;
    VTR_ASSERT(from_model_port);
    VTR_ASSERT(to_model_port);

    //Determine if from_blk and one of it's sinks match the pattern

    if (netlist.block_model(from_blk) != from_blk_model) {
        return false; //Wrong from block type
    }

    for (AtomPinId from_blk_pin : netlist.block_output_pins(from_blk)) {

        AtomPortId from_blk_port = netlist.pin_port(from_blk_pin);
        if (netlist.port_model(from_blk_port) != from_model_port) {
            continue; //Wrong from port
        }

        if (netlist.pin_port_bit(from_blk_pin) != from_port_pin) {
            continue; //Wrong pin within port
        }

        //We have a matching from pin
        //
        //Next, look at all the connected net's associated sink's for 
        //matching to pins
        AtomNetId net = netlist.pin_net(from_blk_pin);
        VTR_ASSERT(from_blk_pin == netlist.net_driver(net));

        for (AtomPinId to_blk_pin : netlist.net_sinks(net)) {
            AtomBlockId to_blk = netlist.pin_block(to_blk_pin);

            if (netlist.block_model(to_blk) != to_blk_model) {
                continue; //Wrong to block type
            }

            AtomPortId to_blk_port = netlist.pin_port(to_blk_pin);
            if (netlist.port_model(to_blk_port) != to_model_port) {
                continue; //Wrong to port
            }

            if (netlist.pin_port_bit(to_blk_pin) != to_port_pin) {
                continue; //Wrong pin within port
            }

            //Match
            from_pin = from_blk_pin;
            to_pin = to_blk_pin;
            return true;
        }
    }

    return false; //No match found
}

void MoleculeDFA::write_dot(std::ostream& os) {

    os << "digraph DFA {\n";
    for (size_t istate = 0; istate < states_.size(); ++istate) {

        os << "\tS" << istate << " [";
        if ((int)istate == start_state_) {
            os << " shape=point ";
        } else if (states_[istate].accepting) {
            os << " shape=doublecircle ";
        } else {
            os << " shape=circle ";
        }
        os << "];\n";

    }

    for (size_t itrans = 0; itrans < transitions_.size(); ++itrans) {
        os << "\tS" << transitions_[itrans].from_state << " -> S" << transitions_[itrans].next_state;
        os << " [";
        os << "label=\"";
        os << "T" << itrans <<":\\n";
        os << transitions_[itrans].pack_pattern_edge.from_model->name << "." << transitions_[itrans].pack_pattern_edge.from_model_port->name << "[" << transitions_[itrans].pack_pattern_edge.from_port_pin << "]";
        os << "\\n-> ";
        os << transitions_[itrans].pack_pattern_edge.to_model->name << "." << transitions_[itrans].pack_pattern_edge.to_model_port->name << "[" << transitions_[itrans].pack_pattern_edge.to_port_pin << "]";
        os << "\"";
        os << "];\n";
    }

    os << "}";
}
