#ifndef ARCH_UTIL_H
#define ARCH_UTIL_H

#include "physical_types.h"

t_port * findPortByName(const char * name, t_pb_type * pb_type,
		int * high_index, int * low_index);

void SetupEmptyType(struct s_type_descriptor* cb_type_descriptors,
                    t_type_ptr EMPTY_TYPE);

void alloc_and_load_default_child_for_pb_type( INOUTP t_pb_type *pb_type,
		char *new_name, t_pb_type *copy);

void ProcessLutClass(INOUTP t_pb_type *lut_pb_type);


void ProcessMemoryClass(INOUTP t_pb_type *mem_pb_type);

e_power_estimation_method power_method_inherited(
		e_power_estimation_method parent_power_method);

void CreateModelLibrary(OUTP struct s_arch *arch);

void SyncModelsPbTypes(INOUTP struct s_arch *arch,
		INP t_type_descriptor * Types, INP int NumTypes);

void SyncModelsPbTypes_rec(INOUTP struct s_arch *arch,
		INOUTP t_pb_type * pb_type);

void UpdateAndCheckModels(INOUTP struct s_arch *arch);


void primitives_annotation_clock_match(
		t_pin_to_pin_annotation *annotation, t_pb_type * parent_pb_type);
#endif
