#ifndef ARCH_UTIL_H
#define ARCH_UTIL_H

#include "physical_types.h"

class InstPort {
  public:
    static constexpr int UNSPECIFIED = -1;

    InstPort() = default;
    InstPort(std::string str);
    std::string instance_name() const { return instance_.name; }
    std::string port_name() const { return port_.name; }

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
        std::string name = "";
        int low_idx = UNSPECIFIED;
        int high_idx = UNSPECIFIED;
    };

    name_index parse_name_index(std::string str);

    name_index instance_;
    name_index port_;
};

void free_arch(t_arch* arch);

void free_type_descriptors(t_type_descriptor* type_descriptors, int num_type_descriptors);

t_port* findPortByName(const char* name, t_pb_type* pb_type, int* high_index, int* low_index);

void SetupEmptyType(t_type_descriptor* cb_type_descriptors,
                    t_type_ptr EMPTY_TYPE);

void alloc_and_load_default_child_for_pb_type(t_pb_type* pb_type,
                                              char* new_name,
                                              t_pb_type* copy);

void ProcessLutClass(t_pb_type* lut_pb_type);

void ProcessMemoryClass(t_pb_type* mem_pb_type);

e_power_estimation_method power_method_inherited(e_power_estimation_method parent_power_method);

void CreateModelLibrary(t_arch* arch);

void SyncModelsPbTypes(t_arch* arch,
                       const t_type_descriptor* Types,
                       const int NumTypes);

void SyncModelsPbTypes_rec(t_arch* arch,
                           t_pb_type* pb_type);

void UpdateAndCheckModels(t_arch* arch);

void primitives_annotation_clock_match(t_pin_to_pin_annotation* annotation,
                                       t_pb_type* parent_pb_type);

bool segment_exists(const t_arch* arch, std::string name);
const t_segment_inf* find_segment(const t_arch* arch, std::string name);
bool is_library_model(const char* model_name);
bool is_library_model(const t_model* model);
#endif
