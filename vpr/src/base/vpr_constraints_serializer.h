#ifndef VPR_CONSTRAINTS_SERIALIZER_H_
#define VPR_CONSTRAINTS_SERIALIZER_H_

#include "region.h"
#include "vpr_constraints.h"
#include "partition.h"
#include "partition_region.h"
#include "echo_files.h"
#include "constraints_load.h"
#include "vtr_log.h"
#include "vtr_error.h"
#include "globals.h" //for the g_vpr_ctx
#include "clock_modeling.h"

#include "vpr_constraints_uxsdcxx_interface.h"

/**
 * @file
 * @brief The reading of vpr constraints is now done via uxsdcxx and the 'vpr_constraints.xsd' file.
 *        The interface between the generated code and VPR is provided by VprConstraintsBase, which is in the
 *        file 'vpr/src/base/gen/vpr_constraints_uxsdcxx_interface.h'.
 * 		  This file implements the virtual functions from VprConstraintsBase.
 *
 * Overview
 * ========
 * 'vpr_constraints.xsd' is an XML schema that specifies the desired format for a VPR constraints file.
 * If this schema is changed, the following files must be regenerated: 'vpr/src/base/gen/vpr_constraints_uxsdcxx.h' and
 * 'vpr/src/base/gen/vpr_constraints_uxsdcxx_interface.h'.
 *
 * Instructions to Update the Files
 * ================================
 *
 * 1. Clone https://github.com/duck2/uxsdcxx/
 * 2. Run 'python3 -mpip install --user -r requirements.txt'
 * 3. Run 'python3 uxsdcxx.py vpr/src/base/vpr_constraints.xsd'
 * 4. Copy 'vpr_constraints_uxsdcxx.h' and 'vpr_constraints_uxsdcxx_interface.h' to vpr/src/base/gen
 * 5. Run 'make format'
 * 6. Update 'vpr/src/base/vpr_constraints_serializer.h' (this file) by adding or changing functions as needed.
 *    If the schema has changed, the compiler will complain that virtual functions are missing if they are
 *    not implemented here.
 *
 * Functions in this file
 * ======================
 *
 * This file implements all the functions that are necessary for the load/read interface of uxsdcxx. These are start_load, finish_load,
 * error_encountered, and any function starting with 'set', 'add', 'preallocate' or 'finish'. Some of the load functions (ex. finish_load and
 * the preallocate functions) have been stubbed out because the load could be successfully completed without implementing them.
 *
 * The functions related to the write interface have also been stubbed out because writing vpr_constraints XML files is not needed at this time.
 * These functions can be implemented to make the write interface if needed.
 *
 * For more detail on how the load and write interfaces work with uxsdcxx, refer to 'vpr/src/route/SCHEMA_GENERATOR.md'
 */

/*
 * Used for the PartitionReadContext, which is used when writing out a constraints XML file.
 * Groups together the information needed when printing a partition.
 */
struct partition_info {
    Partition part;
    std::vector<AtomBlockId> atoms;
    PartitionId part_id;
};

/*
 * The contexts that end with "ReadContext" are used when writing out the XML file.
 * The contexts that end with "WriteContext" are used when reading in the XML file.
 */
struct VprConstraintsContextTypes : public uxsd::DefaultVprConstraintsContextTypes {
    using AddAtomReadContext = AtomBlockId;
    using AddRegionReadContext = Region;
    using PartitionReadContext = partition_info;
    using PartitionListReadContext = void*;
    using SetGlobalSignalReadContext = std::pair<std::string, RoutingScheme>;
    using GlobalRouteConstraintsReadContext = void*;
    using VprConstraintsReadContext = void*;
    using AddAtomWriteContext = void*;
    using AddRegionWriteContext = void*;
    using PartitionWriteContext = void*;
    using PartitionListWriteContext = void*;
    using SetGlobalSignalWriteContext = void*;
    using GlobalRouteConstraintsWriteContext = void*;
    using VprConstraintsWriteContext = void*;
};

class VprConstraintsSerializer final : public uxsd::VprConstraintsBase<VprConstraintsContextTypes> {
  public:
    VprConstraintsSerializer()
        : report_error_(nullptr) {}
    VprConstraintsSerializer(VprConstraints constraints)
        : constraints_(constraints)
        , report_error_(nullptr) {}

    void start_load(const std::function<void(const char*)>* report_error_in) final {
        // report_error_in should be invoked if VprConstraintsSerializer encounters
        // an error during the read.
        report_error_ = report_error_in;
    }

    virtual void start_write() final {}
    virtual void finish_write() final {}

    // error_encountered will be invoked by the reader implementation whenever
    // any error is encountered.
    //
    // This method should **not** be invoked from within VprConstraintsSerializer,
    // instead the error should be reported via report_error.  This enables
    // the reader implementation to add context (e.g. file and line number).
    void error_encountered(const char* file, int line, const char* message) final {
        vpr_throw(VPR_ERROR_OTHER, file, line, "%s", message);
    }

    //resets the routing scheme defined within loaded_route_constraint to the default values
    void reset_loaded_route_constrints() {
        // The network name in the routing scheme needs to be reset to its default value
        // before loading a new constraint for error-checking purposes when the route model
        // is set to dedicated network. The "network_name" attribute is an optional argument
        // but must be specified for use with dedicated network routing. Before loading the
        // constraint, this parameter is set to the default value "INVALID". If, after loading
        // the constraint, this parameter still has its default value but the route model
        // is set to dedicated network, VPR will throw an error.
        loaded_route_constraint.second.reset();
    }

    /** Generated for complex type "add_atom":
     * <xs:complexType name="add_atom">
     *   <xs:attribute name="name_pattern" type="xs:string" use="required" />
     * </xs:complexType>
     */
    virtual inline const char* get_add_atom_name_pattern(AtomBlockId& blk_id) final {
        auto& atom_ctx = g_vpr_ctx.atom();
        temp_atom_string_ = atom_ctx.nlist.block_name(blk_id);
        return temp_atom_string_.c_str();
    }

    virtual inline void set_add_atom_name_pattern(const char* name_pattern, void*& /*ctx*/) final {
        auto& atom_ctx = g_vpr_ctx.atom();
        std::string atom_name = name_pattern;

        auto atom_name_regex = std::regex(atom_name);

        atoms_.clear();

        atom_id_ = atom_ctx.nlist.find_block(name_pattern);

        /* The constraints file may either provide a specific atom name or a regex.
         * If the a valid atom ID is found for the atom name, then a specific atom name
         * must have been read in from the file. The if condition checks for this case.
         * The else statement checks for atoms that may match a regex.
         * This code may get slow if many regexes are given in the file.
         */
        if (atom_id_ != AtomBlockId::INVALID()) {
            atoms_.push_back(atom_id_);
        } else {
            /*If the atom name returns an invalid ID, it might be a regular expression, so loop through the atoms blocks
             * and see if any block names match atom_name_regex.
             */
            for (auto block_id : atom_ctx.nlist.blocks()) {
                auto block_name = atom_ctx.nlist.block_name(block_id);

                if (std::regex_search(block_name, atom_name_regex)) {
                    atoms_.push_back(block_id);
                }
            }
        }

        /*If the atoms_ vector is empty by this point, no atoms were found that matched the name,
         * so the name is invalid.
         */
        if (atoms_.empty()) {
            VTR_LOG_WARN("Atom %s was not found, skipping atom.\n", name_pattern);
        }
    }

    /** Generated for complex type "add_region":
     * <xs:complexType name="add_region">
     *   <xs:attribute name="x_low" type="xs:int" use="required" />
     *   <xs:attribute name="y_low" type="xs:int" use="required" />
     *   <xs:attribute name="x_high" type="xs:int" use="required" />
     *   <xs:attribute name="y_high" type="xs:int" use="required" />
     *   <xs:attribute name="layer_low" type="xs:int" />
     *   <xs:attribute name="layer_high" type="xs:int" />
     *   <xs:attribute name="subtile" type="xs:int" />
     * </xs:complexType>
     */
    virtual inline int get_add_region_layer_low(Region& r) final {
        return r.get_layer_range().first;
    }

    virtual inline int get_add_region_layer_high(Region& r) final {
        return r.get_layer_range().second;
    }

    virtual inline int get_add_region_subtile(Region& r) final {
        return r.get_sub_tile();
    }

    virtual inline void set_add_region_subtile(int subtile, void*& /*ctx*/) final {
        loaded_region.set_sub_tile(subtile);
    }

    virtual inline void set_add_region_layer_high(int layer, void*& /*ctx*/) final {
        auto [layer_low, layer_high] = loaded_region.get_layer_range();
        layer_high = layer;
        loaded_region.set_layer_range({layer_low, layer_high});
    }

    virtual inline void set_add_region_layer_low(int layer, void*& /*ctx*/) final {
        auto [layer_low, layer_high] = loaded_region.get_layer_range();
        layer_low = layer;
        loaded_region.set_layer_range({layer_low, layer_high});
    }

    virtual inline int get_add_region_x_high(Region& r) final {
        return r.get_rect().xmax();
    }

    virtual inline int get_add_region_x_low(Region& r) final {
        return r.get_rect().xmin();
    }

    virtual inline int get_add_region_y_high(Region& r) final {
        return r.get_rect().ymax();
    }

    virtual inline int get_add_region_y_low(Region& r) final {
        return r.get_rect().ymin();
    }

    /** Generated for complex type "partition":
     * <xs:complexType name="partition">
     *   <xs:sequence>
     *     <xs:element maxOccurs="unbounded" name="add_atom" type="add_atom" />
     *     <xs:element maxOccurs="unbounded" name="add_region" type="add_region" />
     *   </xs:sequence>
     *   <xs:attribute name="name" type="xs:string" use="required" />
     * </xs:complexType>
     */
    virtual inline const char* get_partition_name(partition_info& part_info) final {
        temp_part_string_ = part_info.part.get_name();
        return temp_part_string_.c_str();
    }
    virtual inline void set_partition_name(const char* name, void*& /*ctx*/) final {
        loaded_partition.set_name(name);
    }

    virtual inline void preallocate_partition_add_atom(void*& /*ctx*/, size_t /*size*/) final {}

    virtual inline void* add_partition_add_atom(void*& /*ctx*/) final {
        return nullptr;
    }

    virtual inline void finish_partition_add_atom(void*& /*ctx*/) final {
        PartitionId part_id(num_partitions_);

        for (auto atom : atoms_) {
            constraints_.mutable_place_constraints().add_constrained_atom(atom, part_id);
        }
    }

    virtual inline size_t num_partition_add_atom(partition_info& part_info) final {
        return part_info.atoms.size();
    }
    virtual inline AtomBlockId get_partition_add_atom(int n, partition_info& part_info) final {
        return part_info.atoms[n];
    }

    virtual inline void preallocate_partition_add_region(void*& /*ctx*/, size_t /*size*/) final {}

    virtual inline void* add_partition_add_region(void*& /*ctx*/, int x_high, int x_low, int y_high, int y_low) final {
        loaded_region.set_rect({x_low, y_low, x_high, y_high});

        return nullptr;
    }

    virtual inline void finish_partition_add_region(void*& /*ctx*/) final {
        const auto [layer_low, layer_high] = loaded_region.get_layer_range();

        if (layer_low < 0 || layer_high < 0 || layer_high < layer_low) {
            if (report_error_ == nullptr) {
                VPR_ERROR(VPR_ERROR_PLACE, "\nIllegal layer numbers are specified in the constraint file.\n");
            } else {
                report_error_->operator()("Illegal layer numbers are specified in the constraint file.");
            }
        }

        if (loaded_region.empty()) {
            if (report_error_ == nullptr) {
                VPR_ERROR(VPR_ERROR_PLACE, "\nThe specified region is empty.\n");
            } else {
                report_error_->operator()("The specified region is empty.");
            }
        }

        loaded_part_region.add_to_part_region(loaded_region);

        Region clear_region;
        loaded_region = clear_region;
    }

    virtual inline size_t num_partition_add_region(partition_info& part_info) final {
        const PartitionRegion& pr = part_info.part.get_part_region();
        const std::vector<Region>& regions = pr.get_regions();
        return regions.size();
    }
    virtual inline Region get_partition_add_region(int n, partition_info& part_info) final {
        const PartitionRegion& pr = part_info.part.get_part_region();
        const std::vector<Region>& regions = pr.get_regions();
        return regions[n];
    }

    /** Generated for complex type "partition_list":
     * <xs:complexType name="partition_list">
     *   <xs:sequence>
     *     <xs:element maxOccurs="unbounded" name="partition" type="partition" />
     *   </xs:sequence>
     * </xs:complexType>
     */
    virtual inline void preallocate_partition_list_partition(void*& /*ctx*/, size_t /*size*/) final {}

    virtual inline void* add_partition_list_partition(void*& /*ctx*/) final {
        return nullptr;
    }

    virtual inline void finish_partition_list_partition(void*& /*ctx*/) final {
        loaded_partition.set_part_region(loaded_part_region);

        //clear loaded_part_region
        PartitionRegion clear_pr;
        loaded_part_region = clear_pr;

        constraints_.mutable_place_constraints().add_partition(loaded_partition);

        num_partitions_++;
    }
    virtual inline size_t num_partition_list_partition(void*& /*ctx*/) final {
        return constraints_.place_constraints().get_num_partitions();
    }

    /*
     * The argument n is the partition id. Get all the data for partition n so it can be
     * written out.
     */
    virtual inline partition_info get_partition_list_partition(int n, void*& /*ctx*/) final {
        PartitionId partid(n);
        Partition part = constraints_.place_constraints().get_partition(partid);
        std::vector<AtomBlockId> atoms = constraints_.place_constraints().get_part_atoms(partid);

        partition_info part_info;
        part_info.part = part;
        part_info.part_id = partid;
        part_info.atoms = atoms;

        return part_info;
    }

    /** Generated for complex type "set_global_signal":
     * <xs:complexType name="set_global_signal">
     *   <xs:attribute name="name" type="xs:string" use="required" />
     *   <xs:attribute name="network_name" type="xs:string" use="required" />
     *   <xs:attribute name="route_model" type="xs:string" use="required" />
     * </xs:complexType>
     */

    e_clock_modeling from_uxsd_route_model(uxsd::enum_route_model_type route_model) {
        switch (route_model) {
            case uxsd::enum_route_model_type::DEDICATED_NETWORK:
                return e_clock_modeling::DEDICATED_NETWORK;
            case uxsd::enum_route_model_type::ROUTE:
                return e_clock_modeling::ROUTED_CLOCK;
            case uxsd::enum_route_model_type::IDEAL:
                return e_clock_modeling::IDEAL_CLOCK;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                "Invalid route model %d", route_model);
        }
    }

    uxsd::enum_route_model_type to_uxsd_route_model(e_clock_modeling route_model) {
        switch (route_model) {
            case e_clock_modeling::DEDICATED_NETWORK:
                return uxsd::enum_route_model_type::DEDICATED_NETWORK;
            case e_clock_modeling::ROUTED_CLOCK:
                return uxsd::enum_route_model_type::ROUTE;
            case e_clock_modeling::IDEAL_CLOCK:
                return uxsd::enum_route_model_type::IDEAL;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                "Invalid route model %d", route_model);
        }
    }

    virtual inline const char* get_set_global_signal_name(std::pair<std::string, RoutingScheme>& rc) final {
        temp_name_string_ = rc.first;
        return temp_name_string_.c_str();
    }
    virtual inline void set_set_global_signal_name(const char* name, void*& /*ctx*/) final {
        std::string net_name = std::string(name);
        loaded_route_constraint.first = net_name;
        return;
    }
    virtual inline uxsd::enum_route_model_type get_set_global_signal_route_model(std::pair<std::string, RoutingScheme>& rc) final {
        return to_uxsd_route_model(rc.second.route_model());
    }
    virtual inline void set_set_global_signal_route_model(uxsd::enum_route_model_type route_model, void*& /*ctx*/) final {
        loaded_route_constraint.second.set_route_model(from_uxsd_route_model(route_model));
    }
    virtual inline const char* get_set_global_signal_network_name(std::pair<std::string, RoutingScheme>& rc) final {
        temp_name_string_ = rc.second.network_name();
        return temp_name_string_.c_str();
    }
    virtual inline void set_set_global_signal_network_name(const char* network_name, void*& /*ctx*/) final {
        loaded_route_constraint.second.set_network_name(std::string(network_name));
    }

    /** Generated for complex type "global_route_constraints":
     * <xs:complexType name="global_route_constraints">
     *   <xs:sequence>
     *     <xs:element name="set_global_signal" type="set_global_signal" maxOccurs="unbounded" />
     *   </xs:sequence>
     * </xs:complexType>
     */
    virtual inline void preallocate_global_route_constraints_set_global_signal(void*& /*ctx*/, size_t /*size*/) final {}
    virtual inline void* add_global_route_constraints_set_global_signal(void*& ctx, uxsd::enum_route_model_type route_model) final {
        reset_loaded_route_constrints();
        set_set_global_signal_route_model(route_model, ctx);
        return nullptr;
    }
    virtual inline void finish_global_route_constraints_set_global_signal(void*& /*ctx*/) final {
        if (loaded_route_constraint.second.route_model() == e_clock_modeling::DEDICATED_NETWORK
            && loaded_route_constraint.second.network_name() == "INVALID") {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Invalid routing constraint for net \"%s\". The network name has to be specified when routing model is set to \"dedicated_network\".\n", loaded_route_constraint.first.c_str());
        }
        constraints_.mutable_route_constraints().add_route_constraint(loaded_route_constraint.first, loaded_route_constraint.second);
    }
    virtual inline size_t num_global_route_constraints_set_global_signal(void*& /*ctx*/) final {
        return constraints_.route_constraints().get_num_route_constraints();
    }
    virtual inline std::pair<std::string, RoutingScheme> get_global_route_constraints_set_global_signal(int n, void*& /*ctx*/) final {
        return constraints_.route_constraints().get_route_constraint_by_idx((std::size_t)n);
    }

    virtual inline void* init_vpr_constraints_global_route_constraints(void*& /*ctx*/) final {
        return nullptr;
    }

    virtual inline void finish_vpr_constraints_global_route_constraints(void*& /*ctx*/) final {}

    virtual inline void* get_vpr_constraints_global_route_constraints(void*& /*ctx*/) final {
        return nullptr;
    }

    virtual inline bool has_vpr_constraints_global_route_constraints(void*& /*ctx*/) {
        if (constraints_.route_constraints().get_num_route_constraints() > 0)
            return true;
        else
            return false;
    }

    /** Generated for complex type "vpr_constraints":
     * <xs:complexType xmlns:xs="http://www.w3.org/2001/XMLSchema">
     *      <xs:all minOccurs="0">
     *       <xs:element name="partition_list" type="partition_list" />
     *       <xs:element name="global_route_constraints" type="global_route_constraints" />
     *     </xs:all>
     *     <xs:attribute name="tool_name" type="xs:string" />
     *   </xs:complexType>
     */
    virtual inline const char* get_vpr_constraints_tool_name(void*& /*ctx*/) final {
        return temp_.c_str();
    }

    virtual inline void set_vpr_constraints_tool_name(const char* /*tool_name*/, void*& /*ctx*/) final {}

    virtual inline void set_vpr_constraints_constraints_comment(const char* /*constraints_comment*/, void*& /*ctx*/) final {}

    virtual inline const char* get_vpr_constraints_constraints_comment(void*& /*ctx*/) final {
        return temp_.c_str();
    }
    virtual inline void* init_vpr_constraints_partition_list(void*& /*ctx*/) final {
        return nullptr;
    }

    virtual inline void finish_vpr_constraints_partition_list(void*& /*ctx*/) final {
    }

    virtual inline void* get_vpr_constraints_partition_list(void*& /*ctx*/) final {
        return nullptr;
    }

    virtual inline bool has_vpr_constraints_partition_list(void*& /*ctx*/) final {
        if (constraints_.place_constraints().get_num_partitions() > 0)
            return true;
        else
            return false;
    }
    virtual void finish_load() final {
    }

    //temp data for writes
    std::string temp_atom_string_;
    std::string temp_part_string_;
    std::string temp_name_string_;

    /*
     * Temp data for loads and writes.
     * During loads, constraints_ is filled in based on the data read in from the XML file.
     * During writes, constraints_ contains the data that is to be written out to the XML file,
     * and is passed in via the constructor.
     */
    VprConstraints constraints_;
    const std::function<void(const char*)>* report_error_;

    //temp data structures to be loaded during file reading
    Region loaded_region;
    Partition loaded_partition;
    PartitionRegion loaded_part_region;
    std::pair<std::string, RoutingScheme> loaded_route_constraint;

    //temp string used when a method must return a const char*
    std::string temp_ = "vpr";

    //used to count the number of partitions read in from the file
    int num_partitions_ = 0;

    //used when reading in atom names and regular expressions for atoms
    AtomBlockId atom_id_;
    std::vector<AtomBlockId> atoms_;
};

#endif /* VPR_CONSTRAINTS_SERIALIZER_H_ */
