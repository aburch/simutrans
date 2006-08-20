#include "../../tpl/slist_tpl.h"

#include "../intliste_besch.h"
#include "obj_node.h"

#include "intlist_writer.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      intlist_writer_t::write_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      FILE *fp
//      obj_node_t &parent
//      const cstring_t &image_keys
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void intlist_writer_t::write_obj(FILE *fp, obj_node_t &parent, const slist_tpl <int> &values)
{
    intliste_besch_t besch;
    uint16 u;

    besch.anzahl = values.count();

    obj_node_t	node(this, sizeof(besch) + besch.anzahl * sizeof(u), &parent, true);

    node.write_data_at(fp, &besch, 0, sizeof(besch));
    slist_iterator_tpl<int> iter(values);

    int offset = sizeof(besch);
    while(iter.next()) {
	u = iter.get_current();
	node.write_data_at(fp, &u, offset, sizeof(u));
	offset += sizeof(u);
    }
    node.write(fp);
}
