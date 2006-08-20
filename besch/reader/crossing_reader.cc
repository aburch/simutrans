//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  crossing_reader.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: crossing_reader.cpp  $       $Author: hajo $
//  $Revision: 1.3 $         $Date: 2002/09/28 14:47:40 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: crossing_reader.cc,v $
//  Revision 1.3  2002/09/28 14:47:40  hajo
//  Hajo: restructurings
//
//  Revision 1.2  2002/09/25 19:31:17  hajo
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

#include <stdio.h>

#include "../../bauer/wegbauer.h"

#include "../kreuzung_besch.h"
#include "crossing_reader.h"



//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      crossing_reader_t::register_obj()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      char *&data
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void crossing_reader_t::register_obj(obj_besch_t *&data)
{
    kreuzung_besch_t *besch = static_cast<kreuzung_besch_t *>(data);

    wegbauer_t::register_besch(besch);
//    printf("...Baum %s geladen\n", besch->gib_name());
}



bool crossing_reader_t::successfully_loaded() const
{
    return wegbauer_t::alle_kreuzungen_geladen();
}
