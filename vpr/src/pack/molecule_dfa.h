#ifndef MOLECULE_DFA_H
#define MOLECULE_DFA_H
#include <set>
#include <vector>
#include <iosfwd>

#include "atom_netlist_fwd.h"
#include "cad_types.h"

class MoleculeDFA {
    public:
        struct Match {
            struct NetlistEdge {
                NetlistEdge(AtomPinId from, AtomPinId to)
                    : from_pin(from), to_pin(to) {}

                AtomPinId from_pin;
                AtomPinId to_pin;

                friend bool operator<(const NetlistEdge& lhs, const NetlistEdge& rhs) {
                    return std::tie(lhs.from_pin, lhs.to_pin) <
                           std::tie(rhs.from_pin, rhs.to_pin);
                }
                friend bool operator==(const NetlistEdge& lhs, const NetlistEdge& rhs) {
                    return std::tie(lhs.from_pin, lhs.to_pin) ==
                           std::tie(rhs.from_pin, rhs.to_pin);
                }
            };

            Match() = default;
            Match(AtomBlockId blk)
                : root(blk) {}

            AtomBlockId root;
            std::vector<NetlistEdge> netlist_edges;

            int accepted_state = OPEN;

            friend bool operator<(const Match& lhs, const Match& rhs) {
                return std::tie(lhs.root, lhs.netlist_edges, lhs.accepted_state) <
                       std::tie(rhs.root, rhs.netlist_edges, rhs.accepted_state);
            }
            friend bool operator==(const Match& lhs, const Match& rhs) {
                return std::tie(lhs.root, lhs.netlist_edges, lhs.accepted_state) ==
                       std::tie(rhs.root, rhs.netlist_edges, rhs.accepted_state);
            }
        };

    public:
        //Returns all possible matches
        std::set<Match> match_all(const AtomNetlist& netlist) const;

        //Returns the largest possible matches
        //std::set<Match> match_largest(const AtomNetlist& netlist) const;

        //Dump the DFA states/transitions in DOT format
        void write_dot(std::ostream& os);
    public:
        int add_state(bool accepting);
        int add_state_transition(int from_state, int to_state, t_netlist_pack_pattern_edge pattern_edge);
        void set_start_state(int state);

    private:
        struct StateNode {
            StateNode(bool accepts)
                : accepting(accepts) {}

            //Is this an accepting/matching node?
            bool accepting = false;

            //Out-going transitions from this state
            std::vector<int> transitions;
        };

        struct StateTransition {
            t_netlist_pack_pattern_edge pack_pattern_edge; //The pack pattern specification which must match

            int from_state;
            int next_state;
        };

        //Performs a single-step update of all current matches (advances them to their next states)
        std::vector<std::set<Match>> dfa_step(const std::vector<std::set<Match>>& current_state, const AtomNetlist& netlist) const;

        //Returns whether old_match can be extended with pattern_edge
        //If returns true, new_match is the extended version of old_match
        bool matches(Match& new_match, const Match& old_match, const t_netlist_pack_pattern_edge& pattern_edge, const AtomNetlist& netlist) const;

        //Returns whether pattern_edge matches starting at from_blk
        //If true, from_pin and to_pin are updated to the matching pins
        bool block_matches(AtomPinId& from_pin, AtomPinId& to_pin, const AtomBlockId from_blk,
                           const t_netlist_pack_pattern_edge& pattern_edge, const AtomNetlist& netlist) const;

        //Adds any fully matched (accepted) matches to accepted_matches
        void collect_accepted(std::set<Match>& accepted_matches, const std::vector<std::set<Match>>& current_matches) const;

    private:
        int start_state_;
        std::vector<StateNode> states_;
        std::vector<StateTransition> transitions_;
};

#endif
