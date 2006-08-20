//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  imagelist_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: imagelist_writer.cpp $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2004/02/03 19:26:12 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: imagelist_writer.cc,v $
//  Revision 1.3  2004/02/03 19:26:12  hajo
//  Hajo: sync for Hendrik
//
//  Revision 1.2  2002/09/25 19:31:18  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../../utils/cstring_t.h"
#include "../../tpl/slist_tpl.h"

#include "../bildliste_besch.h"
#include "obj_node.h"
#include "image_writer.h"
#include "obj_pak_exception.h"

#include "imagelist_writer.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      imagelist_writer_t::write_obj()
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
void imagelist_writer_t::write_obj(FILE *fp, obj_node_t &parent, const slist_tpl <cstring_t> &keys)
{
    bildliste_besch_t besch;

    obj_node_t	node(this, sizeof(besch), &parent, true);

    slist_iterator_tpl< cstring_t > iter(keys);

	int count = 0;
    while(iter.next()) {
    	if(iter.get_current().chars()==0) {
    		break;
    }
	image_writer_t::instance()->write_obj(fp, node, iter.get_current());
	count ++;
  }
  if(count<keys.count())
  {
	printf("WARNING: Expected %i images, but found only %i (but might be still correct)!\n",keys.count(),count );
	fflush(NULL);
  }
    besch.anzahl = count;//keys.count();

    node.write_data(fp, &besch);
    node.write(fp);
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
