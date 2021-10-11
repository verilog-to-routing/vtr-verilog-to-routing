#ifndef READ_COMMON_FUNC_H
#define READ_COMMON_FUNC_H

#include "arch_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* function declarations */
bool check_model_clocks(t_model*, const char*, uint32_t);
bool check_model_combinational_sinks(const t_model*, const char*, uint32_t);
void warn_model_missing_timing(const t_model*, const char*, uint32_t);

#ifdef __cplusplus
}
#endif

#endif
