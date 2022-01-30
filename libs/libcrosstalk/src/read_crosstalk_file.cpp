#include <string.h>
#include <string>
#include <fstream>
#include <algorithm>

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_memory.h"
#include "vtr_digest.h"
#include "vtr_token.h"
#include "vtr_bimap.h"

#include "read_crosstalk_file.h"


/*
FORMAT (ignore whitespaces/binary):
```
CHANX_CNT CHANY_CNT CHAN_WIDTH
VALUE_0 VALUE_1 VALUE_2 ... --CHANX_0_0
VALUE_0 VALUE_1 VALUE_2 ... --CHANY_0_0
VALUE_0 VALUE_1 VALUE_2 ... --CHANX_0_1
...
VALUE_0 VALUE_1 VALUE_2 ... --CHANY_i_j
```
*/
void CrosstalkSourceRead(const char* CrosstalkFile, t_crosstalk_source* crosstalk){

    VTR_ASSERT(CrosstalkFile != nullptr);
    VTR_ASSERT(crosstalk != nullptr);

	FILE* file = fopen(CrosstalkFile, "rb");
    if(file == nullptr) return;

	int cx, cy, d, a, b;
	float ct;
	//FORMAT: CHANX CHANY DIR A B CT
	while(fscanf(file, "%d %d %d %d %d %f\n", &cx, &cy, &d, &a, &b, &ct) == 6){
		t_ctrs_src cv = {
			{cx,cy,d},{a,b},ct
		};
		crosstalk->push_back(cv);
	};
	fclose(file);
}

void CrosstalkLibRead(const char* CrosstalkFile,
                 t_crosstalk_lib* crosstalk){

    VTR_ASSERT(CrosstalkFile != nullptr);
    VTR_ASSERT(crosstalk != nullptr);

	std::fstream file;
	file.open(CrosstalkFile, std::ios::in | std::ios::binary);

	int node_pair_count = 0;
	file.read(reinterpret_cast<char *>(&node_pair_count), sizeof(node_pair_count));

	int i,idA,idB;
	float v;
	for(i = 0; i < node_pair_count; ++i){
		file.read(reinterpret_cast<char *>(&idA), sizeof(int));
		file.read(reinterpret_cast<char *>(&idB), sizeof(int));
		file.read(reinterpret_cast<char *>(&v), sizeof(float));
		crosstalk->emplace(std::pair<int,int>(idA,idB),v);
	}
	file.close();
}

void CrosstalkLibWrite(const char* CrosstalkFile,
                 t_crosstalk_lib* crosstalk){

    VTR_ASSERT(CrosstalkFile != nullptr);
    VTR_ASSERT(crosstalk != nullptr);

	std::fstream file;
	file.open (CrosstalkFile, std::ios::out | std::ios::binary);

	int node_pair_count = crosstalk->size();
	file.write(reinterpret_cast<char *>(&node_pair_count), sizeof(node_pair_count));

	t_crosstalk_lib::iterator i;
	crosstalk_nodeId_pair cp;
	float v;
	for (i = crosstalk->begin(); i != crosstalk->end(); i++){
		cp = i->first;
		v = i->second;
		file.write(reinterpret_cast<char *>(&(cp.first)), sizeof(int));
		file.write(reinterpret_cast<char *>(&(cp.second)), sizeof(int));
		file.write(reinterpret_cast<char *>(&v), sizeof(float));
	}
	file.close();
}