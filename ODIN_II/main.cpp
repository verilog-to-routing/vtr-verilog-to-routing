#include "odin_ii.h"
#include "types.h"
#include "vtr_time.h"
//This is the main Function of ODIN_II
int main(int argc, char **argv)
{
	vtr::ScopedFinishTimer t("Odin II");
	start_odin_ii(argc,argv);
	terminate_odin_ii();
	return 0;

}