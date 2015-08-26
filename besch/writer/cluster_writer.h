#ifndef CLUSTER_WRITER_H
#define CLUSTER_WRITER_H

#include <stdio.h>
#include "obj_writer.h"
#include "../objversion.h"

class obj_node_t;

class cluster_writer_t 
{
	public:
		static uint32 get_cluster_data(tabfileobj_t& obj, const char* cluster_descriptions);
};

#endif
