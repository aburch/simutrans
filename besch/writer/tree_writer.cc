#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "../baum_besch.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist2d_writer.h"

#include "tree_writer.h"


void tree_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    obj_node_t	node(this, 4, &parent, false);	/* false, because we write this ourselves */
    write_head(fp, node, obj);

    // Hajodoc: Preferred height of this tree type
    // Hajoval: int (useful range: 0-14)
    baum_besch_t besch;
    besch.hoehenlage = obj.get_int("height", 0);
    besch.distribution_weight = obj.get_int("distributionweight", 3);

    slist_tpl< slist_tpl<cstring_t> > keys;

    for(unsigned int age = 0; ; age++) {
	for(int h = 0; ; h++) {
	    char buf[40];

	    // Hajodoc: Images of the tree
	    // Hajoval: int age, int h (age in 0..4, h usually 0)
	    sprintf(buf, "image[%d][%d]", age, h);

	    cstring_t str = obj.get(buf);
	    if(str.len() == 0) {
		break;
	    }
	    if(h == 0) {
		keys.append( slist_tpl<cstring_t>() );
	    }
	    keys.at(age).append(str);
	}
	if(keys.count() <= age) {
	    break;
	}
    }
    imagelist2d_writer_t::instance()->write_obj(fp, node, keys);

	// Hajo: old code
	// node.write_data(fp, &besch);

	// Hajo: temp vars of appropriate size
	uint16 v16;
	uint8 v8;

	// Hajo: write version data
	v16 = 0x8001;
	node.write_data_at(fp, &v16, 0, sizeof(uint16));

	v8 = (uint8) besch.hoehenlage;
	node.write_data_at(fp, &v8, 2, sizeof(uint8));

	v8 = (uint8)besch.distribution_weight;
	node.write_data_at(fp, &v8, 3, sizeof(uint8));

	node.write(fp);
}
