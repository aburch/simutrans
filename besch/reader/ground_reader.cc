//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  ground_reader.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: ground_reader.cpp    $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:17 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: ground_reader.cc,v $
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include <stdio.h>

#include "../../simdings.h"

#include "../grund_besch.h"
#include "ground_reader.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      ground_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      obj_besch_t *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void ground_reader_t::register_obj(obj_besch_t *&data)
{
    grund_besch_t *besch = static_cast<grund_besch_t *>(data);

    grund_besch_t::register_besch(besch);
//    printf("...Grund %s geladen\n", besch->gib_name());
}

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      ground_reader_t::successfully_loaded()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Return type:
//      bool
/////////////////////////////////////////////////////////////////////////////
//@EDOC
bool ground_reader_t::successfully_loaded() const
{
    return grund_besch_t::alles_geladen();
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
