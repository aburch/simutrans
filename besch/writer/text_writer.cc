//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  text_writer.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: MakeObj                      Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: text_writer.cpp      $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:18 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: text_writer.cc,v $
//  Revision 1.2  2002/09/25 19:31:18  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include <string.h>
#include "../../utils/cstring_t.h"

#include "../text_besch.h"
#include "obj_node.h"

#include "text_writer.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      text_writer_t::write_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      FILE *outfp
//      obj_node_t &parent
//      cstring_t text
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void text_writer_t::write_obj(FILE *outfp, obj_node_t &parent, const char *text)
{
    if(!text) {
	text = "";
    }
    int len = strlen(text);

    obj_node_t	node(this, len + 1, &parent, false);

    node.write_data(outfp, text);
    node.write(outfp);
}


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      text_writer_t::dump_node()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      FILE *outfp
//      obj_node_t &node
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void text_writer_t::dump_node(FILE *infp, const obj_node_info_t &node)
{
    obj_writer_t::dump_node(infp, node);

    char *buf = new char[node.size];

    fread(buf, node.size, 1, infp);
    printf(" '%s'", buf);
    delete buf;
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
