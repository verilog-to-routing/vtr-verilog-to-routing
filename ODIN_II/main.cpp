#include "odin_ii.h"
#include "odin_types.h"
#include "vtr_time.h"

int main(int argc, char **argv)
{
	vtr::ScopedFinishTimer t("Odin II");
	struct netlist_t_t *odin_netlist = start_odin_ii(argc, argv);
	terminate_odin_ii(odin_netlist);
	return 0;

}