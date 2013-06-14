#ifndef READ_XML_ARCH_FILE_H
#define READ_XML_ARCH_FILE_H

#include "util.h"
#include "arch_types.h"

#ifdef __cplusplus 
extern "C" {
#endif

/* special type indexes, necessary for initialization, everything afterwards
 should use the pointers to these type indices*/

#define NUM_MODELS_IN_LIBRARY 4
#define EMPTY_TYPE_INDEX 0
#define IO_TYPE_INDEX 1

/* function declarations */
void
XmlReadArch( INP const char *ArchFile, INP boolean timing_enabled,
		OUTP struct s_arch *arch, OUTP t_type_descriptor ** Types,
		OUTP int *NumTypes);
void
EchoArch( INP const char *EchoFile, INP const t_type_descriptor * Types,
		INP int NumTypes, struct s_arch *arch);


#ifdef __cplusplus 
}
#endif

#endif

