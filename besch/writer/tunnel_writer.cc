//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  tunnel_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: tunnel_writer.cpp    $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:18 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: tunnel_writer.cc,v $
//  Revision 1.2  2002/09/25 19:31:18  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

/////////////////////////////////////////////////////////////////////////////
//
//  static data
//
/////////////////////////////////////////////////////////////////////////////

#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "../../dataobj/ribi.h"

#include "../tunnel_besch.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "skin_writer.h"

#include "tunnel_writer.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      tunnel_writer_t::write_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      FILE *fp
//      obj_node_t &parent
//      const char *name
//      slist_tpl<cstring_t> images
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void tunnel_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    tunnel_besch_t besch;
    int pos, i;

    obj_node_t	node(this, sizeof(besch), &parent, true);

    write_head(fp, node, obj);

    static const char * indices[] = {
	"n", "s", "e", "w"
    };
    slist_tpl<cstring_t> backkeys;
    slist_tpl<cstring_t> frontkeys;

    for(pos = 0; pos < 2; pos++) {
	for(i = 0; i < 4; i++) {
	    char buf[40];

	    sprintf(buf, "%simage[%s]", pos ? "back" : "front", indices[i]);
	    cstring_t str = obj.get(buf);
	    (pos ? &backkeys : &frontkeys)->append(str);
	}
    }

    slist_tpl<cstring_t> cursorkeys;
    cursorkeys.append(cstring_t(obj.get("cursor")));
    cursorkeys.append(cstring_t(obj.get("icon")));

    imagelist_writer_t::instance()->write_obj(fp, node, backkeys);
    imagelist_writer_t::instance()->write_obj(fp, node, frontkeys);
    cursorskin_writer_t::instance()->write_obj(fp, node, obj, cursorkeys);

    cursorkeys.clear();
    backkeys.clear();
    frontkeys.clear();

    node.write_data(fp, &besch);
    node.write(fp);
}
