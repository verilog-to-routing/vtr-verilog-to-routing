#ifndef READ_CROSSTALK_FILE_H
#define READ_CROSSTALK_FILE_H



#ifdef __cplusplus
#include <fstream>
extern "C" {
#endif

/*   Detailed routing architecture */
typedef struct {
	int p[3]; //cx, cy, dir
	int w[2]; //a,b
	float v; //ctv
}t_ctrs_src;

typedef std::pair<int,int> crosstalk_nodeId_pair;
typedef std::map<crosstalk_nodeId_pair,float> t_crosstalk_lib;

typedef std::vector<t_ctrs_src> t_crosstalk_source;


/* function declarations */

void CrosstalkSourceRead(const char* CrosstalkFile, 
				 t_crosstalk_source* crosstalk);

void CrosstalkLibRead(const char* CrosstalkFile,
                 t_crosstalk_lib* crosstalk);
void CrosstalkLibWrite(const char* CrosstalkFile,
                 t_crosstalk_lib* crosstalk);

/* macro for SN/TN Net file */

#define ctrs_read_file(file, target) \
	do{ \
		if(file == "") continue; \
		std::ifstream f(file); \
    	std::stringstream buffer; \
    	buffer << f.rdbuf(); \
    	target = buffer.str(); \
	}while(0)


#ifdef __cplusplus
}
#endif

#endif