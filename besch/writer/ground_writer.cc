//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  ground_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: ground_writer.cpp    $       $Author: hajo $
//  $Revision: 1.6 $         $Date: 2003/07/23 19:55:53 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: ground_writer.cc,v $
//  Revision 1.6  2003/07/23 19:55:53  hajo
//  Hajo: sync for Volker
//
//  Revision 1.5  2003/03/26 20:59:24  hajo
//  Hajo: sync with other developers
//
//  Revision 1.4  2003/02/26 09:41:37  hajo
//  Hajo: sync for 0.81.23exp
//
//  Revision 1.3  2003/02/02 10:15:42  hajo
//  Hajo: sync for 0.81.21exp
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../../dataobj/tabfile.h"

#include "obj_node.h"
#include "../grund_besch.h"
#include "text_writer.h"
#include "imagelist2d_writer.h"

#include "ground_writer.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      ground_writer_t::write_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      FILE *fp
//      obj_node_t &parent
//      tabfileobj_t &obj
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void ground_writer_t::write_obj(FILE *fp, obj_node_t &parent, tabfileobj_t &obj)
{
    grund_besch_t besch;

    obj_node_t	node(this, sizeof(besch), &parent, true);

    write_head(fp, node, obj);

    slist_tpl< slist_tpl<cstring_t> > keys;
    // summer images
    for(int hangtyp = 0; hangtyp < 54; hangtyp++) {
	keys.append( slist_tpl<cstring_t>() );

	for(int phase = 0; ; phase++) {
	    char buf[40];

	    sprintf(buf, "image[%d][%d]", hangtyp, phase);

	    cstring_t str = obj.get(buf);
	    if(str.len() == 0) {
		break;
	    }
	    keys.at(hangtyp).append(str);
	}
    }
    imagelist2d_writer_t::instance()->write_obj(fp, node, keys);

    node.write_data(fp, &besch);
    node.write(fp);
}



/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
