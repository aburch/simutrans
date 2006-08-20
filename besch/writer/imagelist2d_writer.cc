//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  imagelist2d_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: imagelist2d_writer.cp$       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:17 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: imagelist2d_writer.cc,v $
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../../utils/cstring_t.h"
#include "../../tpl/slist_tpl.h"

#include "../bildliste2d_besch.h"
#include "obj_node.h"
#include "imagelist_writer.h"

#include "imagelist2d_writer.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      imagelist2d_writer_t::write_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      FILE *fp
//      obj_node_t &parent
//      const slist_tpl<cstring_t> &keys
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void imagelist2d_writer_t::write_obj(FILE *fp, obj_node_t &parent,
				     const slist_tpl< slist_tpl<cstring_t> > &keys)
{
    bildliste2d_besch_t besch;

    obj_node_t	node(this, sizeof(besch), &parent, true);

    slist_iterator_tpl< slist_tpl<cstring_t> > iter(keys);

    besch.anzahl = keys.count();

    while(iter.next()) {
	imagelist_writer_t::instance()->write_obj(fp, node, iter.get_current());
    }
    node.write_data(fp, &besch);
    node.write(fp);
}
