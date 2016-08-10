#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "read_xml_util.h"
#include "read_xml_arch_file.h"

using namespace std;
using namespace pugiutil;

/* Convert bool to ReqOpt enum */ 
extern ReqOpt BoolToReqOpt(bool b) {
	if (b) {
		return REQUIRED;	
	}
	return OPTIONAL;
}

