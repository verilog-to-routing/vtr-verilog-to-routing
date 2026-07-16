#pragma once
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

#include <algorithm>
#include <regex>
#include <unordered_map>
#include "region.h"
#include "vpr_constraints.h"
#include "vtr_assert.h"
#include "partition.h"
#include "partition_region.h"
#include "vpr_context.h"
#include "vtr_log.h"
#include "globals.h" //for the g_vpr_ctx
#include "clock_modeling.h"

#include "vpr_constraints_uxsdcxx_interface.h"

/*
 * Used for the PartitionReadContext, which is used when writing out a constraints XML file.
 * Groups together the information needed when printing a partition.
 */
struct partition_info {
    Partition part;
    std::vector<AtomBlockId> atoms;
    // NOTE: Although lb_types is stored in a set throughout VPR, this needs to be stored in a
    //       vector here. This struct is only used when writing the constraints to a file,
    //       and when this happens this information needs a strict order.
    std::vector<t_logical_block_type_ptr> lb_types;
    PartitionId part_id;
};

/*
 * The contexts that end with "ReadContext" are used when writing out the XML file.
 * The contexts that end with "WriteContext" are used when reading in the XML file.
 */
struct VprConstraintsContextTypes : public uxsd::DefaultVprConstraintsContextTypes {
    using AddAtomReadContext = AtomBlockId;
    using AddRegionReadContext = Region;
    using AddLogicalBlockReadContext = t_logical_block_type_ptr;
    using PartitionReadContext = partition_info;
    using PartitionListReadContext = void*;
    // (macro id, group index) pairs identify a group when writing out relative macros
    using ReferenceGroupReadContext = std::pair<UserRelativeMacroId, int>;
    using RelativeGroupReadContext = std::pair<UserRelativeMacroId, int>;
    using RelativeMacroReadContext = UserRelativeMacroId;
    using RelativeMacroListReadContext = void*;
    using SetGlobalSignalReadContext = std::pair<std::string, RoutingScheme>;
    using GlobalRouteConstraintsReadContext = void*;
    using VprConstraintsReadContext = void*;
    using AddAtomWriteContext = void*;
    using AddRegionWriteContext = void*;
    using AddLogicalBlockWriteContext = void*;
    using PartitionWriteContext = void*;
    using PartitionListWriteContext = void*;
    using ReferenceGroupWriteContext = void*;
    using RelativeGroupWriteContext = void*;
    using RelativeMacroWriteContext = void*;
    using RelativeMacroListWriteContext = void*;
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
     *   <xs:attribute name="is_regex" type="xs:boolean" default="false" />
     *   <xs:attribute name="logical_block_location" type="xs:string" use="optional" />
     * </xs:complexType>
     */
    virtual inline const char* get_add_atom_name_pattern(AtomBlockId& blk_id) final {
        auto& atom_ctx = g_vpr_ctx.atom();
        temp_atom_string_ = atom_ctx.netlist().block_name(blk_id);
        return temp_atom_string_.c_str();
    }

    // we don't need to set is_regex when we write back the XML file as we only write the atom name, not regex patterns, to the XML file
    virtual inline const char* get_add_atom_is_regex(AtomBlockId& /*blk_id*/) final {
        return "false";
    }

    virtual inline void set_add_atom_name_pattern(const char* name_pattern, void*& /*ctx*/) final {
        name_pattern_ = name_pattern;
    }

    virtual inline const char* get_add_atom_logical_block_location(AtomBlockId& blk_id) final {
        // The generated writer skips this optional attribute when nullptr is returned
        temp_logical_block_location_string_ = constraints_.place_constraints().get_atom_logical_block_location(blk_id);
        return temp_logical_block_location_string_.empty() ? nullptr : temp_logical_block_location_string_.c_str();
    }

    virtual inline void set_add_atom_logical_block_location(const char* logical_block_location, void*& /*ctx*/) final {
        logical_block_location_ = logical_block_location;
    }

    virtual inline void set_add_atom_is_regex(const char* is_regex, void*& /*ctx*/) final {
        std::string val = is_regex;
        std::transform(val.begin(), val.end(), val.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (val == "true" || val == "1") {
            is_regex_ = true;
        } else if (val == "false" || val == "0") {
            is_regex_ = false;
        } else {
            VPR_THROW(VPR_ERROR_OTHER,
                      "Invalid value '%s' for add_atom is_regex. Expected true/false or 1/0.",
                      is_regex);
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

    /** Generated for complex type "add_logical_block":
     * <xs:complexType name="add_logical_block">
     *   <xs:attribute name="name_pattern" type="xs:string" use="required" />
     * </xs:complexType>
     */
    virtual inline const char* get_add_logical_block_name_pattern(t_logical_block_type_ptr& logical_block_type) final {
        return logical_block_type->name.c_str();
    }

    // we don't need to set is_regex when we write back the XML file as we only write the logical block type name, not regex patterns, to the XML file
    virtual inline const char* get_add_logical_block_is_regex(t_logical_block_type_ptr& /*logical_block_type*/) final {
        return "false";
    }

    virtual inline void set_add_logical_block_name_pattern(const char* name_pattern, void*& /*ctx*/) final {
        lb_type_name_pattern_ = name_pattern;
    }

    virtual inline void set_add_logical_block_is_regex(const char* is_regex, void*& /*ctx*/) final {
        std::string val = is_regex;
        std::transform(val.begin(), val.end(), val.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (val == "true" || val == "1") {
            lb_type_is_regex_ = true;
        } else if (val == "false" || val == "0") {
            lb_type_is_regex_ = false;
        } else {
            VPR_THROW(VPR_ERROR_OTHER,
                      "Invalid value '%s' for add_logical_block is_regex. Expected true/false or 1/0.",
                      is_regex);
        }
    }

    /** Generated for complex type "partition":
     * <xs:complexType name="partition">
     *   <xs:sequence>
     *      <xs:choice maxOccurs="unbounded">
     *          <xs:element name="add_atom" type="add_atom" />
     *          <xs:element name="add_region" type="add_region" />
     *          <xs:element name="add_logical_block" type="add_logical_block" />
     *      </xs:choice>
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
        //clear out the temporary data for this atom
        name_pattern_.clear();
        logical_block_location_.clear();
        is_regex_ = false;
        return nullptr;
    }

    virtual inline void finish_partition_add_atom(void*& /*ctx*/) final {
        PartitionId part_id(num_partitions_);
        auto& atom_ctx = g_vpr_ctx.atom();
        bool found = false;

        if (!is_regex_) { //the name pattern is not a regex, look for an exact match for the atom name
            AtomBlockId atom_id = atom_ctx.netlist().find_block(name_pattern_);
            if (atom_id != AtomBlockId::INVALID()) {
                found = true;
                constraints_.mutable_place_constraints().add_constrained_atom(atom_id, part_id);
                if (!logical_block_location_.empty()) {
                    constraints_.mutable_place_constraints().set_atom_logical_block_location(atom_id, logical_block_location_);
                }
            }

        } else { //the name pattern is a regex, look for all atoms matching the regex pattern
            auto atom_name_regex = std::regex(name_pattern_);
            for (auto block_id : atom_ctx.netlist().blocks()) { // loop through all block names and add the names that matches with the name_pattern
                auto block_name = atom_ctx.netlist().block_name(block_id);

                if (std::regex_search(block_name, atom_name_regex)) {
                    constraints_.mutable_place_constraints().add_constrained_atom(block_id, part_id);
                    if (!logical_block_location_.empty()) {
                        constraints_.mutable_place_constraints().set_atom_logical_block_location(block_id, logical_block_location_);
                    }
                    found = true;
                }
            }
        }

        if (!found) {
            VTR_LOG_WARN("Atom %s was not found, skipping atom.\n", name_pattern_.c_str());
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

    virtual inline void preallocate_partition_add_logical_block(void*& /*ctx*/, size_t /*size*/) final {}

    virtual inline void* add_partition_add_logical_block(void*& /*ctx*/) final {
        //clear out the temporary data for this logical block name
        lb_type_name_pattern_.clear();
        lb_type_is_regex_ = false;
        return nullptr;
    }

    virtual inline void finish_partition_add_logical_block(void*& /*ctx*/) final {
        const auto& device_ctx = g_vpr_ctx.device();
        PartitionId part_id(num_partitions_);
        bool found = false;

        if (!lb_type_is_regex_) { //the logical block type name pattern is not a regex, look for an exact match for the logical block type name
            for (const t_logical_block_type& logical_block_type : device_ctx.logical_block_types) {
                if (logical_block_type.name == lb_type_name_pattern_) {
                    constraints_.mutable_place_constraints().constrain_part_lb_type(part_id, &logical_block_type);
                    found = true;
                }
            }
        } else { //the logical block type name pattern is a regex, look for all logical block types matching the regex pattern
            auto lb_type_name_regex = std::regex(lb_type_name_pattern_);
            for (const t_logical_block_type& logical_block_type : device_ctx.logical_block_types) { // loop through all logical block type names and add the names that matches with the name_pattern

                if (std::regex_search(logical_block_type.name, lb_type_name_regex)) {
                    constraints_.mutable_place_constraints().constrain_part_lb_type(part_id, &logical_block_type);
                    found = true;
                }
            }
        }

        if (!found) {
            VTR_LOG_WARN("Logical block type %s was not found, skipping logical block type.\n",
                         lb_type_name_pattern_.c_str());
            return;
        }
    }

    virtual inline size_t num_partition_add_logical_block(partition_info& part_info) final {
        return part_info.lb_types.size();
    }

    virtual inline t_logical_block_type_ptr get_partition_add_logical_block(int n, partition_info& part_info) final {
        return part_info.lb_types[n];
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
        if (constraints_.place_constraints().is_part_constrained_to_lb_types(partid)) {
            const std::unordered_set<t_logical_block_type_ptr>& lb_types_set = constraints_.place_constraints().get_part_lb_type_constraints(partid);
            part_info.lb_types.assign(lb_types_set.begin(), lb_types_set.end());
        }

        return part_info;
    }

    /** Generated for complex type "reference_group":
     * <xs:complexType name="reference_group">
     *   <xs:sequence>
     *     <xs:element name="add_atom" type="add_atom" maxOccurs="unbounded" />
     *   </xs:sequence>
     * </xs:complexType>
     */
    virtual inline void preallocate_reference_group_add_atom(void*& /*ctx*/, size_t /*size*/) final {}

    virtual inline void* add_reference_group_add_atom(void*& /*ctx*/) final {
        //clear out the temporary data for this atom
        name_pattern_.clear();
        logical_block_location_.clear();
        is_regex_ = false;
        return nullptr;
    }

    virtual inline void finish_reference_group_add_atom(void*& /*ctx*/) final {
        resolve_atoms_into_loaded_relative_group();
    }

    virtual inline size_t num_reference_group_add_atom(std::pair<UserRelativeMacroId, int>& group_ctx) final {
        return get_relative_group_from_ctx(group_ctx).atoms.size();
    }
    virtual inline AtomBlockId get_reference_group_add_atom(int n, std::pair<UserRelativeMacroId, int>& group_ctx) final {
        return get_relative_group_from_ctx(group_ctx).atoms[n];
    }

    /** Generated for complex type "relative_group":
     * <xs:complexType name="relative_group">
     *   <xs:sequence>
     *     <xs:element name="add_atom" type="add_atom" maxOccurs="unbounded" />
     *   </xs:sequence>
     *   <xs:attribute name="x_offset" type="xs:int" use="required" />
     *   <xs:attribute name="y_offset" type="xs:int" use="required" />
     *   <xs:attribute name="sub_tile_offset" type="xs:int" use="required" />
     *   <xs:attribute name="layer_offset" type="xs:int" />
     * </xs:complexType>
     */
    virtual inline int get_relative_group_x_offset(std::pair<UserRelativeMacroId, int>& group_ctx) final {
        return get_relative_group_from_ctx(group_ctx).offset.x;
    }
    virtual inline int get_relative_group_y_offset(std::pair<UserRelativeMacroId, int>& group_ctx) final {
        return get_relative_group_from_ctx(group_ctx).offset.y;
    }
    virtual inline int get_relative_group_sub_tile_offset(std::pair<UserRelativeMacroId, int>& group_ctx) final {
        return get_relative_group_from_ctx(group_ctx).offset.sub_tile;
    }
    virtual inline int get_relative_group_layer_offset(std::pair<UserRelativeMacroId, int>& group_ctx) final {
        return get_relative_group_from_ctx(group_ctx).offset.layer;
    }

    virtual inline void set_relative_group_layer_offset(int layer_offset, void*& /*ctx*/) final {
        loaded_relative_group_.offset.layer = layer_offset;
    }

    virtual inline void preallocate_relative_group_add_atom(void*& /*ctx*/, size_t /*size*/) final {}

    virtual inline void* add_relative_group_add_atom(void*& /*ctx*/) final {
        //clear out the temporary data for this atom
        name_pattern_.clear();
        logical_block_location_.clear();
        is_regex_ = false;
        return nullptr;
    }

    virtual inline void finish_relative_group_add_atom(void*& /*ctx*/) final {
        resolve_atoms_into_loaded_relative_group();
    }

    virtual inline size_t num_relative_group_add_atom(std::pair<UserRelativeMacroId, int>& group_ctx) final {
        return get_relative_group_from_ctx(group_ctx).atoms.size();
    }
    virtual inline AtomBlockId get_relative_group_add_atom(int n, std::pair<UserRelativeMacroId, int>& group_ctx) final {
        return get_relative_group_from_ctx(group_ctx).atoms[n];
    }

    /** Generated for complex type "relative_macro":
     * <xs:complexType name="relative_macro">
     *   <xs:sequence>
     *     <xs:element name="reference_group" type="reference_group" />
     *     <xs:element name="relative_group" type="relative_group" maxOccurs="unbounded" />
     *   </xs:sequence>
     *   <xs:attribute name="name" type="xs:string" use="required" />
     * </xs:complexType>
     */
    virtual inline const char* get_relative_macro_name(UserRelativeMacroId& macro_id) final {
        temp_macro_string_ = constraints_.relative_macros().get_macro(macro_id).name;
        return temp_macro_string_.c_str();
    }
    virtual inline void set_relative_macro_name(const char* name, void*& /*ctx*/) final {
        loaded_relative_macro_.name = name;
    }

    virtual inline void* init_relative_macro_reference_group(void*& /*ctx*/) final {
        //the reference group is the anchor of the macro: implicit zero offset
        loaded_relative_group_ = UserRelativeGroup();
        return nullptr;
    }

    virtual inline void finish_relative_macro_reference_group(void*& /*ctx*/) final {
        //the reference group is always groups[0], even when it matched no atoms
        //(whether an empty reference group is an error is decided when the whole
        //macro has been read, see finish_relative_macro_list_relative_macro)
        VTR_ASSERT(loaded_relative_macro_.groups.empty());
        loaded_relative_macro_.groups.push_back(loaded_relative_group_);
    }

    virtual inline std::pair<UserRelativeMacroId, int> get_relative_macro_reference_group(UserRelativeMacroId& macro_id) final {
        return {macro_id, 0};
    }

    virtual inline void preallocate_relative_macro_relative_group(void*& /*ctx*/, size_t /*size*/) final {}

    virtual inline void* add_relative_macro_relative_group(void*& /*ctx*/, int sub_tile_offset, int x_offset, int y_offset) final {
        loaded_relative_group_ = UserRelativeGroup();
        loaded_relative_group_.offset.x = x_offset;
        loaded_relative_group_.offset.y = y_offset;
        loaded_relative_group_.offset.sub_tile = sub_tile_offset;
        loaded_relative_group_.offset.layer = 0; //optional attribute, may be overwritten by set_relative_group_layer_offset
        return nullptr;
    }

    virtual inline void finish_relative_macro_relative_group(void*& /*ctx*/) final {
        if (loaded_relative_group_.offset.layer != 0) {
            //cross-layer relative macros are not supported yet: no macro code path
            //exercises nonzero layer offsets, so reject them at load time
            report_relative_macro_error("Relative macro '" + loaded_relative_macro_.name
                                        + "': layer_offset must be 0. Cross-layer relative macros are not supported.");
        }

        if (loaded_relative_group_.atoms.empty()) {
            VTR_LOG_WARN("Relative macro '%s': a relative_group matched no atoms, skipping the group.\n",
                         loaded_relative_macro_.name.c_str());
            return;
        }

        loaded_relative_macro_.groups.push_back(loaded_relative_group_);
    }

    virtual inline size_t num_relative_macro_relative_group(UserRelativeMacroId& macro_id) final {
        //groups[0] is the reference group; the rest are the relative groups
        return constraints_.relative_macros().get_macro(macro_id).groups.size() - 1;
    }
    virtual inline std::pair<UserRelativeMacroId, int> get_relative_macro_relative_group(int n, UserRelativeMacroId& macro_id) final {
        return {macro_id, n + 1};
    }

    /** Generated for complex type "relative_macro_list":
     * <xs:complexType name="relative_macro_list">
     *   <xs:sequence>
     *     <xs:element name="relative_macro" type="relative_macro" maxOccurs="unbounded" />
     *   </xs:sequence>
     * </xs:complexType>
     */
    virtual inline void preallocate_relative_macro_list_relative_macro(void*& /*ctx*/, size_t /*size*/) final {}

    virtual inline void* add_relative_macro_list_relative_macro(void*& /*ctx*/) final {
        loaded_relative_macro_ = UserRelativeMacro();
        return nullptr;
    }

    virtual inline void finish_relative_macro_list_relative_macro(void*& /*ctx*/) final {
        const std::string& macro_name = loaded_relative_macro_.name;
        const std::vector<UserRelativeGroup>& groups = loaded_relative_macro_.groups;
        VTR_ASSERT(!groups.empty()); //the reference group is always present

        //macro names must be unique since they identify macros in error messages
        for (size_t imacro = 0; imacro < constraints_.relative_macros().get_num_macros(); imacro++) {
            if (constraints_.relative_macros().get_macro(UserRelativeMacroId(imacro)).name == macro_name) {
                report_relative_macro_error("Relative macro name '" + macro_name + "' is used more than once. Macro names must be unique.");
            }
        }

        //empty relative groups were dropped when they were read, so any group
        //past the reference group is non-empty
        if (groups[0].atoms.empty()) {
            if (groups.size() == 1) {
                VTR_LOG_WARN("Relative macro '%s': no group matched any atoms, dropping the macro.\n",
                             macro_name.c_str());
            } else {
                report_relative_macro_error("Relative macro '" + macro_name
                                            + "': the reference group matched no atoms but a relative group did. The macro has no anchor.");
            }
            return;
        }

        if (groups.size() == 1) {
            VTR_LOG_WARN("Relative macro '%s': all relative groups matched no atoms, dropping the macro.\n",
                         macro_name.c_str());
            return;
        }

        //two groups at the same offset would require two clusters at the same location
        for (size_t i = 0; i < groups.size(); i++) {
            for (size_t j = i + 1; j < groups.size(); j++) {
                if (groups[i].offset == groups[j].offset) {
                    report_relative_macro_error("Relative macro '" + macro_name + "': groups " + std::to_string(i)
                                                + " and " + std::to_string(j)
                                                + " have identical offsets. Two groups of a macro cannot be placed at the same location.");
                }
            }
        }

        //an atom may belong to at most one group across all relative macros
        const auto& atom_ctx = g_vpr_ctx.atom();
        std::unordered_map<AtomBlockId, size_t> atoms_seen;
        for (size_t igroup = 0; igroup < groups.size(); igroup++) {
            for (AtomBlockId blk_id : groups[igroup].atoms) {
                auto [seen_itr, first_time] = atoms_seen.insert({blk_id, igroup});
                if (!first_time) {
                    report_relative_macro_error("Relative macro '" + macro_name + "': atom '"
                                                + atom_ctx.netlist().block_name(blk_id) + "' appears in groups "
                                                + std::to_string(seen_itr->second) + " and " + std::to_string(igroup)
                                                + ". An atom may belong to at most one relative placement group.");
                }

                auto [other_macro_id, other_group_idx] = constraints_.relative_macros().get_atom_group(blk_id);
                if (other_macro_id.is_valid()) {
                    report_relative_macro_error("Atom '" + atom_ctx.netlist().block_name(blk_id)
                                                + "' appears in relative macro '" + macro_name + "' and in relative macro '"
                                                + constraints_.relative_macros().get_macro(other_macro_id).name
                                                + "'. An atom may belong to at most one relative placement group.");
                }
            }
        }

        constraints_.mutable_relative_macros().add_macro(loaded_relative_macro_);
    }

    virtual inline size_t num_relative_macro_list_relative_macro(void*& /*ctx*/) final {
        return constraints_.relative_macros().get_num_macros();
    }
    virtual inline UserRelativeMacroId get_relative_macro_list_relative_macro(int n, void*& /*ctx*/) final {
        return UserRelativeMacroId(n);
    }

    virtual inline void* init_vpr_constraints_relative_macro_list(void*& /*ctx*/) final {
        return nullptr;
    }

    virtual inline void finish_vpr_constraints_relative_macro_list(void*& /*ctx*/) final {}

    virtual inline void* get_vpr_constraints_relative_macro_list(void*& /*ctx*/) final {
        return nullptr;
    }

    virtual inline bool has_vpr_constraints_relative_macro_list(void*& /*ctx*/) final {
        return constraints_.relative_macros().get_num_macros() > 0;
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

    /**
     * @brief Report an error found while loading relative placement macros.
     */
    void report_relative_macro_error(const std::string& msg) {
        if (report_error_ == nullptr) {
            VPR_ERROR(VPR_ERROR_PLACE, "\n%s\n", msg.c_str());
        } else {
            report_error_->operator()(msg.c_str());
        }
    }

    /**
     * @brief Resolve the current add_atom name pattern (exact or regex, same
     *        semantics as partition atoms) and append the matched atoms to the
     *        relative placement group being loaded.
     */
    void resolve_atoms_into_loaded_relative_group() {
        const auto& atom_ctx = g_vpr_ctx.atom();
        bool found = false;

        if (!is_regex_) { //the name pattern is not a regex, look for an exact match for the atom name
            AtomBlockId atom_id = atom_ctx.netlist().find_block(name_pattern_);
            if (atom_id != AtomBlockId::INVALID()) {
                add_atom_to_loaded_relative_group(atom_id);
                found = true;
            }
        } else { //the name pattern is a regex, look for all atoms matching the regex pattern
            auto atom_name_regex = std::regex(name_pattern_);
            for (AtomBlockId block_id : atom_ctx.netlist().blocks()) {
                if (std::regex_search(atom_ctx.netlist().block_name(block_id), atom_name_regex)) {
                    add_atom_to_loaded_relative_group(block_id);
                    found = true;
                }
            }
        }

        if (!logical_block_location_.empty()) {
            VTR_LOG_WARN("Relative macro '%s': logical_block_location is not supported on relative macro atoms, ignoring it for atom pattern %s.\n",
                         loaded_relative_macro_.name.c_str(), name_pattern_.c_str());
        }

        if (!found) {
            VTR_LOG_WARN("Atom %s was not found, skipping atom.\n", name_pattern_.c_str());
        }
    }

    /**
     * @brief Append an atom to the relative placement group being loaded,
     *        ignoring duplicates (two patterns of a group may match the same atom).
     */
    void add_atom_to_loaded_relative_group(AtomBlockId blk_id) {
        std::vector<AtomBlockId>& atoms = loaded_relative_group_.atoms;
        if (std::find(atoms.begin(), atoms.end(), blk_id) == atoms.end()) {
            atoms.push_back(blk_id);
        }
    }

    /**
     * @brief Return the group identified by a (macro id, group index) read context.
     */
    const UserRelativeGroup& get_relative_group_from_ctx(const std::pair<UserRelativeMacroId, int>& group_ctx) const {
        return constraints_.relative_macros().get_macro(group_ctx.first).groups[group_ctx.second];
    }

    //temp data for writes
    std::string temp_atom_string_;
    std::string temp_part_string_;
    std::string temp_name_string_;
    std::string temp_macro_string_;
    std::string temp_logical_block_location_string_;

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
    UserRelativeMacro loaded_relative_macro_;
    UserRelativeGroup loaded_relative_group_;

    //temp string used when a method must return a const char*
    std::string temp_ = "vpr";

    //used to count the number of partitions read in from the file
    int num_partitions_ = 0;

    //used when reading in atom names and regular expressions for atoms
    bool is_regex_;
    std::string name_pattern_;
    std::string logical_block_location_;

    // Used when reading in regex LB type constraints for a partition.
    bool lb_type_is_regex_;
    std::string lb_type_name_pattern_;
};
