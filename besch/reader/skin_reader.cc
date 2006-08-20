//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  skin_reader.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: skin_reader.cpp      $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:17 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: skin_reader.cc,v $
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include <stdio.h>

#include "../../simdings.h"
#include "../../simskin.h"

#include "../skin_besch.h"
#include "skin_reader.h"


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      skin_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      obj_besch_t *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void skin_reader_t::register_obj(obj_besch_t *&data)
{
    skin_besch_t *besch = reinterpret_cast<skin_besch_t *>(data);

    if(get_skintype() != skinverwaltung_t::nothing)
	skinverwaltung_t::register_besch(get_skintype(), besch);
    else
	obj_for_xref(get_type(), besch->gib_name(), data);
}


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      skin_reader_t::successfully_loaded()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Return type:
//      bool
/////////////////////////////////////////////////////////////////////////////
//@EDOC
bool skin_reader_t::successfully_loaded() const
{
    return skinverwaltung_t::alles_geladen(get_skintype());
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
