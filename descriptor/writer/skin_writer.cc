/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include "../../simdebug.h"
#include "../../dataobj/tabfile.h"
#include "../skin_desc.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"

using std::string;


void skin_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	slist_tpl<string> keys;

	for(  int i = 0;  ;  i++  ) {
		char buf[40];

		sprintf(buf, "image[%d]", i);

		string str = obj.get(buf);
		if(  str.empty()  ) {
			break;
		}
		if(  debuglevel > 2  ) {
			printf("%10sImage[%3u] =%s\n", " ", i, str.c_str());
		}
		keys.append(str);
	}
	write_obj(fp, parent, obj, keys);
}


void skin_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj, const slist_tpl<string>& imagekeys)
{
	slist_tpl<string> keys;

	obj_node_t node(this, 0, &parent);

	write_head(fp, node, obj);

	imagelist_writer_t::instance()->write_obj(fp, node, imagekeys);

	node.write(fp);
}
