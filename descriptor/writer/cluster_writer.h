/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_WRITER_CLUSTER_WRITER_H
#define DESCRIPTOR_WRITER_CLUSTER_WRITER_H


#include <stdio.h>
#include "obj_writer.h"
#include "../objversion.h"

class obj_node_t;

class cluster_writer_t
{
	public:
		static uint32 get_cluster_data(tabfileobj_t& obj, const char* cluster_descriptions)
		{
			{
				uint32 clusters = 0;
				int* ints = obj.get_ints(cluster_descriptions);

				for(  int i = 1;  i <= ints[0];  i++  ) {
					if(  ints[i] >= 1  &&  ints[i] <= 32  ) { // Sanity check
						clusters |= 1<<(ints[i]-1);
					}
				}
				delete [] ints;

				return clusters;
			}
		}
};

#endif
