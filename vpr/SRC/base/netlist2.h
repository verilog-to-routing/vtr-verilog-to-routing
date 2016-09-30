#ifndef NETLIST2_H
#define NETLIST2_H
#include <vector>
#include <unordered_map>

#include "vtr_range.h"
#include "vtr_hash.h"
#include "vtr_logic.h"
#include "vtr_strong_id.h"

#include "logic_types.h" //For t_model

//Forward delcarations
class AtomNetlist;

//Type tags for Ids
struct atom_blk_id_tag;
struct atom_net_id_tag;
struct atom_port_id_tag;
struct atom_pin_id_tag;
struct atom_string_id_tag;

typedef vtr::StrongId<atom_blk_id_tag> AtomBlockId;
typedef vtr::StrongId<atom_net_id_tag> AtomNetId;
typedef vtr::StrongId<atom_port_id_tag> AtomPortId;
typedef vtr::StrongId<atom_pin_id_tag> AtomPinId;
typedef vtr::StrongId<atom_string_id_tag> AtomStringId;

typedef unsigned BitIndex;

enum class AtomPortType : char {
    INPUT,
    OUTPUT,
    CLOCK
};

enum class AtomPinType : char {
    DRIVER,
    SINK
};

enum class AtomBlockType : char {
    INPAD,
    OUTPAD,
    COMBINATIONAL,
    SEQUENTIAL
};

namespace std {
    //Make tuples hashable for std::unordered_map
    template<>
    struct hash<std::tuple<AtomPortId,BitIndex>> {
        std::size_t operator()(const std::tuple<AtomPortId,BitIndex>& k) const {
            std::size_t seed = 0;
            vtr::hash_combine(seed, std::hash<AtomPortId>()(get<0>(k)));
            vtr::hash_combine(seed, std::hash<size_t>()(get<1>(k)));
            return seed;
        }
    };
    template<>
    struct hash<std::tuple<AtomStringId,AtomPortType>> {
        typedef std::underlying_type<AtomPortType>::type enum_type;
        std::size_t operator()(const std::tuple<AtomStringId,AtomPortType>& k) const {
            std::size_t seed = 0;
            vtr::hash_combine(seed, std::hash<AtomStringId>()(get<0>(k)));
            vtr::hash_combine(seed, std::hash<enum_type>()(static_cast<enum_type>(get<1>(k))));
            return seed;
        }
    };
    template<>
    struct hash<std::tuple<AtomBlockId,AtomStringId>> {
        std::size_t operator()(const std::tuple<AtomBlockId,AtomStringId>& k) const {
            std::size_t seed = 0;
            vtr::hash_combine(seed, std::hash<AtomBlockId>()(get<0>(k)));
            vtr::hash_combine(seed, std::hash<AtomStringId>()(get<1>(k)));
            return seed;
        }
    };
} //namespace std

class AtomNetlist {
    public: //Public types
        typedef std::vector<AtomBlockId>::const_iterator block_iterator;
        typedef std::vector<AtomPortId>::const_iterator port_iterator;
        typedef std::vector<AtomPinId>::const_iterator pin_iterator;
        typedef std::vector<AtomNetId>::const_iterator net_iterator;
        typedef std::vector<std::vector<vtr::LogicValue>> TruthTable;
    public:

        //Constructs a netlist
        // name - the name of the netlist
        AtomNetlist(std::string name="");

    public: //Public Accessors
        //Netlist
        const std::string&  netlist_name() const;

        //Block
        const std::string&          block_name          (const AtomBlockId id) const;
        AtomBlockType               block_type          (const AtomBlockId id) const;
        const t_model*              block_model         (const AtomBlockId id) const;
        const TruthTable&           block_truth_table   (const AtomBlockId id) const; 
        vtr::Range<port_iterator>   block_input_ports   (const AtomBlockId id) const;
        vtr::Range<port_iterator>   block_output_ports  (const AtomBlockId id) const;
        vtr::Range<port_iterator>   block_clock_ports   (const AtomBlockId id) const;
        AtomPinId                   block_pin           (const AtomPortId port_id, BitIndex port_bit) const;

        //Port
        const std::string&          port_name   (const AtomPortId id) const;
        BitIndex                    port_width  (const AtomPortId id) const;
        AtomBlockId                 port_block  (const AtomPortId id) const; 
        AtomPortType                port_type   (const AtomPortId id) const; 
        vtr::Range<pin_iterator>    port_pins   (const AtomPortId id) const;

        //Pin
        AtomNetId           pin_net     (const AtomPinId id) const; 
        AtomPinType         pin_type    (const AtomPinId id) const; 
        AtomPortId          pin_port    (const AtomPinId id) const;
        BitIndex            pin_port_bit(const AtomPinId id) const;
        AtomBlockId         pin_block   (const AtomPinId id) const;

        //Net
        const std::string&          net_name    (const AtomNetId id) const; 
        vtr::Range<pin_iterator>    net_pins    (const AtomNetId id) const;
        AtomPinId                   net_driver  (const AtomNetId id) const;
        vtr::Range<pin_iterator>    net_sinks   (const AtomNetId id) const;

        //Aggregates
        vtr::Range<block_iterator>  blocks  () const;
        vtr::Range<net_iterator>    nets    () const;
        
        //Lookups
        AtomBlockId find_block  (const std::string& name) const;
        AtomPortId  find_port   (const AtomBlockId blk_id, const std::string& name) const;
        AtomPinId   find_pin    (const AtomPortId port_id, BitIndex port_bit) const;
        AtomNetId   find_net    (const std::string& name) const;

        //Sanity check for internal consistency
        bool verify() const;

        //Indictes if the netlist has invalid entries due to modification
        bool dirty() const;

        //Item counts and container info (for debugging)
        void print_stats() const;

    public: //Public Mutators
        //Note: all create_*() functions will silently return the appropriate ID if it has already been created
        AtomBlockId create_block(const std::string name, const AtomBlockType blk_type, const t_model* model, const TruthTable truth_table=TruthTable());
        AtomPortId  create_port (const AtomBlockId blk_id, const std::string& name);
        AtomPinId   create_pin  (const AtomPortId port_id, BitIndex port_bit, const AtomNetId net_id, const AtomPinType type);
        AtomNetId   create_net  (const std::string name); //An empty or existing net
        AtomNetId   add_net  (const std::string name, AtomPinId driver, std::vector<AtomPinId> sinks); //A new fully-specified net

        void remove_block(const AtomBlockId blk_id);
        void remove_net(const AtomNetId net_id);
        void remove_net_pin(const AtomNetId net_id, const AtomPinId pin_id);

        //Removes all blocks/ports/pins/nets which are marked as invalid
        //Note: this will cause Ids to change!
        void compress();

    private: //Private types

    private: //Private members
        //Lookups
        AtomStringId find_string(const std::string& str) const;
        AtomBlockId find_block(const AtomStringId name_id) const;
        AtomPortId  find_port(const AtomBlockId blk_id, const AtomStringId name_id) const;
        AtomNetId find_net(const AtomStringId name_id) const;

        const t_model_ports* find_model_port(const AtomPortId id, const std::string& name) const;

        //Mutators
        AtomStringId create_string(const std::string& str);
        void remove_port(const AtomPortId port_id);
        void remove_pin(const AtomPinId pin_id);

        //Netlist compression
        std::vector<AtomBlockId> clean_blocks();
        std::vector<AtomPortId> clean_ports();
        std::vector<AtomPinId> clean_pins();
        std::vector<AtomNetId> clean_nets();

        void rebuild_block_refs(const std::vector<AtomPortId>& port_id_map);
        void rebuild_port_refs(const std::vector<AtomBlockId>& block_id_map, const std::vector<AtomPinId>& pin_id_map);
        void rebuild_pin_refs(const std::vector<AtomPortId>& port_id_map, const std::vector<AtomNetId>& net_id_map);
        void rebuild_net_refs(const std::vector<AtomPinId>& pin_id_map);
        void rebuild_lookups();

        void shrink_to_fit();

        //Sanity Checks
        bool verify_sizes() const;
        bool verify_refs() const;
        bool verify_lookups() const;

        bool valid_block_id(AtomBlockId id) const;
        bool valid_port_id(AtomPortId id) const;
        bool valid_port_bit(AtomPortId id, BitIndex port_bit) const;
        bool valid_pin_id(AtomPinId id) const;
        bool valid_net_id(AtomNetId id) const;
        bool valid_string_id(AtomStringId id) const;

        bool validate_block_sizes() const;
        bool validate_port_sizes() const;
        bool validate_pin_sizes() const;
        bool validate_net_sizes() const;
        bool validate_string_sizes() const;
        bool validate_block_port_refs() const;
        bool validate_port_pin_refs() const;
        bool validate_net_pin_refs() const;
        bool validate_string_refs() const;

    private: //Private data

        //Netlist data
        std::string                 netlist_name_;   //Name of the top-level netlist
        bool                        dirty_; //Indicates the netlist has invalid entries from remove_*() functions

        //Block data
        std::vector<AtomBlockId>             block_ids_;      //Valid block ids
        std::vector<AtomStringId>            block_names_;    //Name of each block
        std::vector<AtomBlockType>           block_types_;    //Type of each block
        std::vector<const t_model*>          block_models_;   //Architecture model of each block
        std::vector<TruthTable>              block_truth_tables_; //Truth tables of each block
        std::vector<std::vector<AtomPortId>> block_input_ports_; //Input ports of each block
        std::vector<std::vector<AtomPortId>> block_output_ports_; //Output ports of each block
        std::vector<std::vector<AtomPortId>> block_clock_ports_; //Clock ports of each block

        //Port data
        std::vector<AtomPortId>             port_ids_;          //Valid port ids
        std::vector<AtomStringId>           port_names_;        //Name of each port
        std::vector<AtomBlockId>            port_blocks_;       //Block associated with each port
        std::vector<std::vector<AtomPinId>> port_pins_;         //Pins associated with each port

        //Pin data
        std::vector<AtomPinId>      pin_ids_;        //Valid pin ids
        std::vector<AtomPortId>     pin_ports_;      //Type of each pin
        std::vector<BitIndex>       pin_port_bits_;  //The ports bit position in the port
        std::vector<AtomNetId>      pin_nets_;       //Net associated with each pin

        //Net data
        std::vector<AtomNetId>              net_ids_;    //Valid net ids
        std::vector<AtomStringId>           net_names_;  //Name of each net
        std::vector<std::vector<AtomPinId>> net_pins_;   //Pins associated with each net

        //String data
        // We store each unique string once, and reference it by an StringId
        // This avoids duplicating the strings in the fast look-ups (i.e. the look-ups
        // only store the Ids)
        std::vector<AtomStringId>   string_ids_;    //Valid string ids
        std::vector<std::string>    strings_;       //Strings

    private: //Fast lookups

        std::unordered_map<AtomStringId,AtomBlockId> block_name_to_block_id_;
        std::unordered_map<std::tuple<AtomBlockId,AtomStringId>,AtomPortId> block_id_port_name_to_port_id_;
        std::unordered_map<std::tuple<AtomPortId,BitIndex>,AtomPinId> pin_port_port_bit_to_pin_id_;
        std::unordered_map<AtomStringId,AtomNetId> net_name_to_net_id_;
        std::unordered_map<std::string,AtomStringId> string_to_string_id_;
};

#endif
