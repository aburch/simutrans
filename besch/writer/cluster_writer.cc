#include "cluster_writer.h"
#include "../../dataobj/tabfile.h"

// Subroutine for write_obj, to avoid duplicated code
uint32 cluster_writer_t::get_cluster_data(tabfileobj_t& obj, const char* cluster_descriptions)
{
	uint32 clusters = 0;
	int* ints = obj.get_ints(cluster_descriptions);

	for(  int i = 1;  i <= ints[0];  i++  ) {
		if(  ints[i] > 1  &&  ints[i] <= 32  ) { // Sanity check
			clusters |= 1<<(ints[i]-1);
		}
	}
	delete [] ints;

	return clusters;
}