#include "read_xml_util.h"

using namespace std;
using namespace pugiutil;

/* Convert bool to ReqOpt enum */ 
extern ReqOpt BoolToReqOpt(bool b) {
	if (b) {
		return REQUIRED;	
	}
	return OPTIONAL;
}

