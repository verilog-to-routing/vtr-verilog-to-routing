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

struct VprConstraintsContextTypes : public uxsd::DefaultVprConstraintsContextTypes {
    using AddAtomReadContext = void*;
    using AddRegionReadContext = void*;
    using PartitionReadContext = void*;
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
    virtual inline const char* get_add_atom_name_pattern(void*& /*ctx*/) final {
        return temp_.c_str();
    }

    virtual inline void set_add_atom_name_pattern(const char* name_pattern, void*& /*ctx*/) final {
        auto& atom_ctx = g_vpr_ctx.atom();
        std::string atom_name = name_pattern;
        atom_id_ = atom_ctx.nlist.find_block(name_pattern);

        if (atom_id_ == AtomBlockId::INVALID()) {
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
    virtual inline int get_add_region_subtile(void*& /*ctx*/) final {
        int i = 0;
        return i;
    }

    virtual inline void set_add_region_subtile(int subtile, void*& /*ctx*/) final {
        loaded_region.set_sub_tile(subtile);
    }

    virtual inline int get_add_region_x_high(void*& /*ctx*/) final {
        int i = 0;
        return i;
    }

    virtual inline int get_add_region_x_low(void*& /*ctx*/) final {
        int i = 0;
        return i;
    }

    virtual inline int get_add_region_y_high(void*& /*ctx*/) final {
        int i = 0;
        return i;
    }

    virtual inline int get_add_region_y_low(void*& /*ctx*/) final {
        int i = 0;
        return i;
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
    virtual inline const char* get_partition_name(void*& /*ctx*/) final {
        return temp_.c_str();
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

        if (atom_id_ != AtomBlockId::INVALID()) {
            constraints_.add_constrained_atom(atom_id_, part_id);
        }
    }

    virtual inline size_t num_partition_add_atom(void*& /*ctx*/) final {
        int i = 0;
        return i;
    }
    virtual inline void* get_partition_add_atom(int /*n*/, void*& /*ctx*/) final {
        return nullptr;
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

    virtual inline size_t num_partition_add_region(void*& /*ctx*/) final {
        int i = 0;
        return i;
    }
    virtual inline void* get_partition_add_region(int /*n*/, void*& /*ctx*/) final {
        return nullptr;
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
        int i = 0;
        return i;
    }

    virtual inline void* get_partition_list_partition(int /*n*/, void*& /*ctx*/) final {
        return nullptr;
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

    //temp data for loads
    const std::function<void(const char*)>* report_error_;
    Region loaded_region;
    Partition loaded_partition;
    PartitionRegion loaded_part_region;
    VprConstraints constraints_;
    std::string temp_;
    int num_partitions_ = 0;
    AtomBlockId atom_id_;
};

#endif /* VPR_CONSTRAINTS_SERIALIZER_H_ */
