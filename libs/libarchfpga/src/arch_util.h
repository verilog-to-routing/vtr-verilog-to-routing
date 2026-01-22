#pragma once

#include <unordered_set>
#include "physical_types.h"

/**
 * @brief sets the architecture file name to be retrieved by the various parser functions
 */
void set_arch_file_name(const char* arch);

/**
 * @brief returns the architecture file name, requires that it was previously set
 */
const char* get_arch_file_name();

constexpr const char* EMPTY_BLOCK_NAME = "EMPTY";

class InstPort {
  public:
    static constexpr int UNSPECIFIED = -1;

    InstPort() = default;
    InstPort(const std::string& str);
    const std::string& instance_name() const { return instance_.name; }
    const std::string& port_name() const { return port_.name; }

    int instance_low_index() const { return instance_.low_idx; }
    int instance_high_index() const { return instance_.high_idx; }
    int port_low_index() const { return port_.low_idx; }
    int port_high_index() const { return port_.high_idx; }

    int num_instances() const;
    int num_pins() const;

  public:
    void set_port_low_index(int val) { port_.low_idx = val; }
    void set_port_high_index(int val) { port_.high_idx = val; }

  private:
    struct name_index {
        std::string name;
        int low_idx = UNSPECIFIED;
        int high_idx = UNSPECIFIED;
    };

    name_index parse_name_index(const std::string& str);

    name_index instance_;
    name_index port_;
};

void free_arch(t_arch* arch);

void free_type_descriptors(std::vector<t_logical_block_type>& type_descriptors);
void free_type_descriptors(std::vector<t_physical_tile_type>& type_descriptors);

t_port* findPortByName(const char* name, t_pb_type* pb_type, int* high_index, int* low_index);

/** @brief Returns and empty physical tile type, assigned with the given name argument.
 *         The default empty string is assigned if no name is provided
 */
t_physical_tile_type get_empty_physical_type(const char* name = EMPTY_BLOCK_NAME);

/** @brief Returns and empty logical block type, assigned with the given name argument.
 *         The default empty string is assigned if no name is provided
 */
t_logical_block_type get_empty_logical_type(const char* name = EMPTY_BLOCK_NAME);

std::unordered_set<t_logical_block_type_ptr> get_equivalent_sites_set(t_physical_tile_type_ptr type);

void alloc_and_load_default_child_for_pb_type(t_pb_type* pb_type,
                                              std::string_view new_name,
                                              t_pb_type* copy);

void ProcessLutClass(t_pb_type* lut_pb_type);

void ProcessMemoryClass(t_pb_type* mem_pb_type);

e_power_estimation_method power_method_inherited(e_power_estimation_method parent_power_method);

void SyncModelsPbTypes(t_arch* arch,
                       const std::vector<t_logical_block_type>& Types);

void SyncModelsPbTypes_rec(t_arch* arch,
                           t_pb_type* pb_type);

void primitives_annotation_clock_match(t_pin_to_pin_annotation* annotation,
                                       t_pb_type* parent_pb_type);

bool segment_exists(const t_arch* arch, std::string_view name);
const t_segment_inf* find_segment(const t_arch* arch, std::string_view name);

//Returns true if the specified block type contains the specified blif model name
bool block_type_contains_blif_model(t_logical_block_type_ptr type, const std::string& blif_model_name);

//Returns true of a pb_type (or it's children) contain the specified blif model name
bool pb_type_contains_blif_model(const t_pb_type* pb_type, const std::string& blif_model_name);

/**
 * @brief Recursive helper method to deduce if the given pb_type is or contains
 *        pb_types which are of the memory class.
 */
bool pb_type_contains_memory_pbs(const t_pb_type* pb_type);

bool has_sequential_annotation(const t_pb_type* pb_type, const t_model_ports* port, enum e_pin_to_pin_delay_annotations annot_type);
bool has_combinational_annotation(const t_pb_type* pb_type, std::string_view in_port, std::string_view out_port);

/**
 * @brief Updates the physical and logical types based on the equivalence between one and the other.
 *
 * This function is required to check and synchronize all the information to be able to use the logical block
 * equivalence, and link all the logical block pins to the physical tile ones, given that multiple logical blocks (i.e. pb_types)
 * can be placed at the same physical location if this is allowed in the architecture description.
 *
 * See https://docs.verilogtorouting.org/en/latest/tutorials/arch/equivalent_sites/ for reference
 */
void link_physical_logical_types(std::vector<t_physical_tile_type>& PhysicalTileTypes,
                                 std::vector<t_logical_block_type>& LogicalBlockTypes);

void setup_pin_classes(t_physical_tile_type* type);
