//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  tree_reader.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: tree_reader.cpp      $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:17 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: tree_reader.cc,v $
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include <stdio.h>

#include "../../simdings.h"
#include "../../dings/baum.h"

#include "../baum_besch.h"
#include "tree_reader.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      tree_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      obj_besch_t *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void tree_reader_t::register_obj(obj_besch_t *&data)
{
    baum_besch_t *besch = static_cast<baum_besch_t *>(data);

    baum_t::register_besch(besch);
//    printf("...Baum %s geladen\n", besch->gib_name());
}


bool tree_reader_t::successfully_loaded() const
{
    return baum_t::alles_geladen();
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
