#include "odin_ii.h"
#include "types.h"
#include "vtr_time.h"

int main(int argc, char **argv)
{
	vtr::ScopedFinishTimer t("Odin II");
	netlist_t *veri_net = start_odin_ii(argc,argv);
	if(!veri_net)
	{
		std::cout << "odin failed to create a netlist for the input";
		return -1;
	}


	int term_error = terminate_odin_ii();
	if(!veri_net)
	{
		std::cout << "odin failed to close gracefully";
		return -1;
	}

	return 0;

}