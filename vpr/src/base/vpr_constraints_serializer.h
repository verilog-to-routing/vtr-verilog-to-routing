#ifndef VPR_CONSTRAINTS_SERIALIZER_H_
#define VPR_CONSTRAINTS_SERIALIZER_H_

#include "region.h"
#include "vpr_constraints.h"
#include "partition.h"
#include "partition_region.h"
#include "echo_files.h"
#include "constraints_load.h"
#include "vtr_log.h"
#include "globals.h" //for the g_vpr_ctx

#include "vpr_constraints_uxsdcxx_interface.h"

/**
 * @file
 * @brief The reading of vpr floorplanning constraints is now done via uxsdcxx and the 'vpr_constraints.xsd' file.
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
    using VprConstraintsReadContext = void*;
    using AddAtomWriteContext = void*;
    using AddRegionWriteContext = void*;
    using PartitionWriteContext = void*;
    using PartitionListWriteContext = void*;
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
     *   <xs:attribute name="subtile" type="xs:int" />
     * </xs:complexType>
     */
    virtual inline int get_add_region_subtile(Region& r) final {
        return r.get_sub_tile();
    }

    virtual inline void set_add_region_subtile(int subtile, void*& /*ctx*/) final {
        loaded_region.set_sub_tile(subtile);
    }

    virtual inline int get_add_region_x_high(Region& r) final {
        vtr::Rect<int> rect = r.get_region_rect();
        return rect.xmax();
    }

    virtual inline int get_add_region_x_low(Region& r) final {
        vtr::Rect<int> rect = r.get_region_rect();
        return rect.xmin();
    }

    virtual inline int get_add_region_y_high(Region& r) final {
        vtr::Rect<int> rect = r.get_region_rect();
        return rect.ymax();
    }

    virtual inline int get_add_region_y_low(Region& r) final {
        vtr::Rect<int> rect = r.get_region_rect();
        return rect.ymin();
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

        for (unsigned int i = 0; i < atoms_.size(); i++) {
            constraints_.add_constrained_atom(atoms_[i], part_id);
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
        loaded_region.set_region_rect(x_low, y_low, x_high, y_high);

        return nullptr;
    }

    virtual inline void finish_partition_add_region(void*& /*ctx*/) final {
        loaded_part_region.add_to_part_region(loaded_region);

        Region clear_region;
        loaded_region = clear_region;
    }

    virtual inline size_t num_partition_add_region(partition_info& part_info) final {
        PartitionRegion pr = part_info.part.get_part_region();
        std::vector<Region> regions = pr.get_partition_region();
        return regions.size();
    }
    virtual inline Region get_partition_add_region(int n, partition_info& part_info) final {
        PartitionRegion pr = part_info.part.get_part_region();
        std::vector<Region> regions = pr.get_partition_region();
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

        constraints_.add_partition(loaded_partition);

        num_partitions_++;
    }
    virtual inline size_t num_partition_list_partition(void*& /*ctx*/) final {
        return constraints_.get_num_partitions();
    }

    /*
     * The argument n is the partition id. Get all the data for partition n so it can be
     * written out.
     */
    virtual inline partition_info get_partition_list_partition(int n, void*& /*ctx*/) final {
        PartitionId partid(n);
        Partition part = constraints_.get_partition(partid);
        std::vector<AtomBlockId> atoms = constraints_.get_part_atoms(partid);

        partition_info part_info;
        part_info.part = part;
        part_info.part_id = partid;
        part_info.atoms = atoms;

        return part_info;
    }

    /** Generated for complex type "vpr_constraints":
     * <xs:complexType xmlns:xs="http://www.w3.org/2001/XMLSchema">
     *     <xs:all>
     *       <xs:element name="partition_list" type="partition_list" />
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

    virtual void finish_load() final {
    }

    //temp data for writes
    std::string temp_atom_string_;
    std::string temp_part_string_;

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

    //temp string used when a method must return a const char*
    std::string temp_ = "vpr";

    //used to count the number of partitions read in from the file
    int num_partitions_ = 0;

    //used when reading in atom names and regular expressions for atoms
    AtomBlockId atom_id_;
    std::vector<AtomBlockId> atoms_;
};

#endif /* VPR_CONSTRAINTS_SERIALIZER_H_ */
