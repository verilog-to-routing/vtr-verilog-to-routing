#ifndef ARCH_UTIL_H
#define ARCH_UTIL_H

#include "physical_types.h"

t_port * findPortByName(const char * name, t_pb_type * pb_type,
		int * high_index, int * low_index);

void SetupEmptyType(struct s_type_descriptor* cb_type_descriptors,
                    t_type_ptr EMPTY_TYPE);

void alloc_and_load_default_child_for_pb_type( t_pb_type *pb_type,
		char *new_name, t_pb_type *copy);

void ProcessLutClass(t_pb_type *lut_pb_type);


void ProcessMemoryClass(t_pb_type *mem_pb_type);

e_power_estimation_method power_method_inherited(
		e_power_estimation_method parent_power_method);

void CreateModelLibrary(struct s_arch *arch);

void SyncModelsPbTypes(struct s_arch *arch,
		const t_type_descriptor * Types, const int NumTypes);

void SyncModelsPbTypes_rec(struct s_arch *arch,
		t_pb_type * pb_type);

void UpdateAndCheckModels(struct s_arch *arch);


void primitives_annotation_clock_match(
		t_pin_to_pin_annotation *annotation, t_pb_type * parent_pb_type);

bool segment_exists(const t_arch* arch, std::string name);
t_segment_inf* find_segment(const t_arch* arch, std::string name);
#endif
