#ifndef NETLIST2_H
#define NETLIST2_H
#include <vector>
#include <unordered_map>

#include "vtr_range.h"
#include "vtr_hash.h"
#include "vtr_logic.h"

#include "logic_types.h" //For t_model

typedef int AtomBlkId;
typedef int AtomNetId;
typedef int AtomPinId;

constexpr AtomBlkId INVALID_ATOM_BLK = -1;
constexpr AtomNetId INVALID_ATOM_NET = -1;
constexpr AtomPinId INVALID_ATOM_PIN = -1;

enum class AtomPinType {
    DRIVER,
    SINK
};

enum class AtomBlockType {
    INPAD,
    OUTPAD,
    COMBINATIONAL,
    SEQUENTIAL
};

//Make a tuple of AtomBlkId, AtomNetId and AtomPinType std::string hashable so we can use std::unordered_map
namespace std {
    template<>
    struct hash<std::tuple<AtomBlkId,AtomNetId,AtomPinType,std::string>> {
        std::size_t operator()(const std::tuple<AtomBlkId,AtomNetId,AtomPinType,std::string>& k) const {
            std::size_t seed = 0;
            vtr::hash_combine(seed, std::hash<AtomBlkId>()(get<0>(k)));
            vtr::hash_combine(seed, std::hash<AtomNetId>()(get<1>(k)));
            vtr::hash_combine(seed, std::hash<int>()(static_cast<int>(get<2>(k))));
            vtr::hash_combine(seed, std::hash<std::string>()(get<3>(k)));
            return seed;
        }
    };
} //namespace std

class AtomNetlist {
    public: //Public types
        typedef std::vector<AtomBlkId>::const_iterator blk_iterator;
        typedef std::vector<AtomPinId>::const_iterator pin_iterator;
        typedef std::vector<AtomNetId>::const_iterator net_iterator;
        typedef std::vector<std::vector<vtr::LogicValue>> TruthTable;
    public:
        AtomNetlist(std::string name);

    public: //Public Accessors
        //Netlist
        const std::string&  netlist_name() const;
        bool                is_blackbox() const;

        //Block
        const std::string&          block_name          (const AtomBlkId id) const;
        AtomBlockType               block_type          (const AtomBlkId id) const;
        const t_model*              block_model         (const AtomBlkId id) const;
        const TruthTable&           block_truth_table   (const AtomBlkId id) const; 
        vtr::Range<pin_iterator>    block_input_pins    (const AtomBlkId id) const;
        vtr::Range<pin_iterator>    block_output_pins   (const AtomBlkId id) const;

        //Pin
        AtomBlkId   pin_block   (const AtomPinId id) const; 
        AtomNetId   pin_net     (const AtomPinId id) const; 
        AtomPinType pin_type    (const AtomPinId id) const; 

        //Net
        const std::string&          net_name    (const AtomNetId id) const; 
        vtr::Range<pin_iterator>    net_pins    (const AtomNetId id) const;
        AtomPinId                   net_driver  (const AtomNetId id) const;
        vtr::Range<pin_iterator>    net_sinks   (const AtomNetId id) const;

        //Aggregates
        vtr::Range<blk_iterator> blocks  () const;
        vtr::Range<pin_iterator> pins    () const;
        vtr::Range<net_iterator> nets    () const;
        
        //Lookups
        AtomNetId   find_net    (const std::string& name) const;
        AtomPinId   find_pin    (const AtomBlkId blk_id, const AtomNetId net_id, const AtomPinType pin_type, const std::string& pin_name) const;
        AtomBlkId   find_block  (const std::string& name) const;

    public: //Public Mutators
        //Note: all create_*() functions will silently return the appropriate ID if it has already been created
        AtomBlkId   create_block(const std::string name, const AtomBlockType blk_type, const t_model* model, const TruthTable truth_table=TruthTable());
        AtomNetId   create_net  (const std::string name);
        AtomPinId   create_pin  (const AtomBlkId blk_id, const AtomNetId net_id, const AtomPinType pin_type, const std::string name);

        void set_blackbox(bool val);
    
    private: //Private types
        typedef int AtomPinNameId;
        static constexpr AtomPinNameId INVALID_PIN_NAME_ID = -1;

    private: //Private members
        //Lookups
        AtomPinNameId find_pin_name_id(const std::string& name);

        //Sanity Checks
        bool valid_block_id(AtomBlkId id) const;
        bool valid_net_id(AtomNetId id) const;
        bool valid_pin_id(AtomPinId id) const;

    private: //Private data

        //Netlist data
        std::string                 netlist_name_;   //Name of the top-level netlist
        bool                        is_blackbox_;    //Indicates this netlist is a black box

        std::vector<AtomBlkId>      block_ids_;      //Valid block ids
        std::vector<std::string>    block_names_;    //Name of each block
        std::vector<AtomBlockType>  block_types_;    //Type of each block
        std::vector<const t_model*> block_models_;   //Architecture model of each block
        std::vector<TruthTable>     block_truth_tables_; //Truth tables of each block

        std::vector<AtomBlkId>      pin_ids_;        //Valid pin ids
        std::vector<AtomBlkId>      pin_blocks_;     //Block associated with each pin
        std::vector<AtomNetId>      pin_nets_;       //Net associated with each pin
        std::vector<AtomPinType>    pin_types_;      //Type of each pin
        std::vector<std::string>    pin_names_;      //Unique pin names (indexed via AtomPinNameId)
        std::vector<AtomPinNameId>  pin_name_ids_;   //Index of pin into pin_names_ (we expect duplicates)

        std::vector<AtomNetId>              net_ids_;    //Valid net ids
        std::vector<std::string>            net_names_;  //Name of each net
        std::vector<std::vector<AtomPinId>> net_pins_;   //Pins associated with each net

        //Fast lookups
        std::unordered_map<std::string,AtomBlkId> block_name_to_id_;
        std::unordered_map<std::string,AtomNetId> net_name_to_id_;
        std::unordered_map<std::string,AtomPinNameId> pin_name_to_name_id_;
        std::unordered_map<std::tuple<AtomBlkId,AtomNetId,AtomPinType,std::string>,AtomPinId> pin_blk_net_type_name_to_id_;

        //Fast iteration of block inputs/outputs
        std::vector<std::vector<AtomPinId>> block_input_pins_;
        std::vector<std::vector<AtomPinId>> block_output_pins_;
};

#endif
