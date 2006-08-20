//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  skin_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: skin_writer.cpp      $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:18 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: skin_writer.cc,v $
//  Revision 1.2  2002/09/25 19:31:18  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"

#include "../skin_besch.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist_writer.h"

#include "skin_writer.h"

void skin_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    slist_tpl<cstring_t> keys;

    for(int i = 0; ;i++) {
	char buf[40];

	sprintf(buf, "image[%d]", i);

	cstring_t str = obj.get(buf);
	if(str.len() == 0) {
	    break;
	}
	keys.append(str);
    }
    write_obj(fp, parent, obj, keys);
}


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      skin_writer_t::write_obj()
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
void skin_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj,
			      const slist_tpl<cstring_t> &imagekeys)
{
    slist_tpl<cstring_t> keys;

    skin_besch_t besch;

    obj_node_t	node(this, sizeof(besch), &parent, true);

    write_head(fp, node, obj);

    imagelist_writer_t::instance()->write_obj(fp, node, imagekeys);

    node.write_data(fp, &besch);
    node.write(fp);
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
