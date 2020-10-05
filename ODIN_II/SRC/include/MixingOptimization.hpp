/*
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef MIXING_OPTIMIZATION_HPP
#define MIXING_OPTIMIZATION_HPP
#include "odin_types.h" // netlist_t, config_t

class HardSoftLogicMixer;
/**
 * @brief A base class in hierarchy for complex synthesis
 * allowing for mixing soft and hard logic 
 */
class MixingOpt {
  public:
    /**
     * @brief Construct a new Mixing Opt object for disabled optimization
     * usable for querying 'hardenable' condition
     */
    MixingOpt() { _enabled = false; }

    /**
     * @brief Construct a new Mixing Opt object
     * 
     * By default, all optimizations only share 
     * the ratio of blocks to be implemented in 
     * hard logic
     * @param ratio, a value within 0 to 1 to 
     * implement ratio*requested hard blocks in
     * hard logic
     * @param kind a kind of blocks that correspond
     * to optimization pass
     */
    MixingOpt(float ratio, operation_list kind)
        : _kind(kind) {
        _ratio = ratio;
    }

    /**
     * @brief Destroy the Mixing Opt object
     * required by compiler
     */
    virtual ~MixingOpt() = default;

    /**
     * @brief assign weights to the candidate nodes vector, according to netlist_statistic
     *
     * @param nnode_t* pointer to the node
     */
    virtual void assign_weights(netlist_t* netlist, std::vector<nnode_t*> nodes);

    /**
     * @brief Checks if the optimization is enabled for this node
     *
     * @param nodes pointer to the vector with mults
     */
    virtual bool enabled() {
        return _enabled;
    }

    /**
     * @brief Instantiates an alternative (not on hard blocks)
     * implementation for the operation
     * 
     * @param netlist 
     * @param nodes 
     */
    virtual void instantiate_soft_logic(netlist_t* netlist, std::vector<nnode_t*> nodes);

    /**
     * @brief performs the optimization pass, varies between kinds.
     * If the implementation is not provided within the inherited class
     * will throw ODIN error
     * 
     * @param netlist_t* pointer to a global netlist
     * @param std::vector<nnode_t*> a vector with nodes the optimization
     * pass is concerned (all of which are potential candidates to 
     * be implemented in hard blocks for a given _kind)
     */
    virtual void perform(netlist_t*, std::vector<nnode_t*>&);

    /**
     * @brief Set the blocks of blocks required
     * by counting in netlist
     *
     * @param count 
     */
    virtual void set_blocks_needed(int count);

    operation_list get_kind() {
        return _kind;
    }

    /**
     * @brief based on criteria for hardening given kind of operation, return
     * if the node should be implemented in hard blocks
     *
     * @param nnode_t* pointer to the node
     */
    virtual bool hardenable(nnode_t*) {
        return false;
    }

    /**
     * @brief allowing for replacing with dynamic polymorphism for different
     * kinds of nodes
     *
     * @param nnode_t* pointer to the node
     */
    virtual void partial_map_node(nnode_t*, short, netlist_t*, HardSoftLogicMixer*);

  protected:
    /**
     * @brief a routine that will multiply
     * required blocks by the ratio
     */
    virtual void scale_counts();

    /**
     * @brief this variable allows to cache traverse value
     * 
     */
    short cached_traverse_value = 0;

    // an integer representing the number of required hard blocks
    // that should be estimated and updated through set blocks needed
    int _blocks_count = -1;
    // a boolean type to double check if the optimization is enabled
    bool _enabled = false;
    // a parameter allowing for scaling counts
    float _ratio = -1.0;
    // an enum kind variable, corresponding to an optimization pass
    operation_list _kind = operation_list_END;
};

class MultsOpt : public MixingOpt {
  public:
    /**
     * @brief Construct a new Mults Opt object for disabled optimization
     * usable for querying 'hardenable' condition
     */
    MultsOpt()
        : MixingOpt() {}

    /**
     * @brief Construct a new Mults Opt object
     * from ratio parameter
     * @param ratio 
     */
    MultsOpt(float ratio);
    /**
     * @brief Construct a new Mults Opt object
     * allowing to set exact number of multipliers
     * that will be used
     * @param exact 
     */
    MultsOpt(int exact);

    /**
     * @brief assign weights to the candidate nodes vector, according to netlist_statistic
     *
     * @param nodes pointer to the vector with mults
     */
    virtual void assign_weights(netlist_t* netlist, std::vector<nnode_t*> nodes);

    /**
     * @brief allowing for replacing with dynamic polymorphism for different
     * kinds of nodes
     *
     * @param nnode_t* pointer to the node
     */
    virtual void partial_map_node(nnode_t*, short, netlist_t*, HardSoftLogicMixer*);
    /**
     * @brief Instantiates an alternative (not on hard blocks)
     * implementation for the operation
     * 
     * @param netlist 
     * @param nodes 
     */
    virtual void instantiate_soft_logic(netlist_t* netlist, std::vector<nnode_t*> nodes);

    /**
     * @brief performs the optimization pass, specifically for multipliers.
     * If the implementation is not provided within the inherited class
     * will throw ODIN error
     * 
     * @param netlist_t* pointer to a global netlist
     * @param std::vector<nnode_t*> a vector with nodes the optimization
     * pass is concerned (all of which are potential candidates to 
     * be implemented in hard blocks for a given _kind)
     */
    virtual void perform(netlist_t* netlist, std::vector<nnode_t*>&);

    /**
     * @brief Set the blocks of blocks required
     * by counting in netlist. Has to be overriden, to account
     * with specifics of optimization
     *
     * @param count 
     */
    virtual void set_blocks_needed(int);

    /**
     * @brief based on criteria for hardening given kind of operation, return
     * if the node should be implemented in hard blocks
     *
     * @param nnode_t* pointer to the node
     */
    virtual bool hardenable(nnode_t*);
};

#endif
