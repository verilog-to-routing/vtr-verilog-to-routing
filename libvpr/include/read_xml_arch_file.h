#ifndef READ_XML_ARCH_FILE_H
#define READ_XML_ARCH_FILE_H

#include "util.h"
#include "arch_types.h"

/* function declarations */
void
XmlReadArch( INP const char *ArchFile, INP boolean timing_enabled,
		OUTP struct s_arch *arch, OUTP t_type_descriptor ** Types,
		OUTP int *NumTypes);
void
EchoArch( INP const char *EchoFile, INP const t_type_descriptor * Types,
		INP int NumTypes, struct s_arch *arch);

#endif

