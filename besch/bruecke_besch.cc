//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  bruecke_besch.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: bruecke_besch.cpp    $       $Author: hajo $
//  $Revision: 1.2 $         $Date: 2002/09/25 19:31:17 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: bruecke_besch.cc,v $
//  Revision 1.2  2002/09/25 19:31:17  hajo
//  Volker: new objects
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

#include "../simdebug.h"

#include "bruecke_besch.h"



/*
 *  member function:
 *      bruecke_besch_t::gib_simple()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Richtigen Index für einfaches Brückenstück bestimmen
 *
 *  Return type:
 *      bruecke_besch_t::img_t
 *
 *  Argumente:
 *      ribi_t::ribi ribi
 */
bruecke_besch_t::img_t bruecke_besch_t::gib_simple(ribi_t::ribi ribi)
{
    return (ribi & ribi_t::nordsued) ? NS_Segment : OW_Segment;
}



/*
 *  member function:
 *      bruecke_besch_t::gib_start()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Richtigen Index für klassischen Hangstart ück bestimmen
 *
 *  Return type:
 *      bruecke_besch_t::img_t
 *
 *  Argumente:
 *      ribi_t::ribi ribi
 */
bruecke_besch_t::img_t bruecke_besch_t::gib_start(ribi_t::ribi ribi)
{
    switch(ribi) {
    case ribi_t::nord:	return N_Start;
    case ribi_t::sued:	return S_Start;
    case ribi_t::ost:	return O_Start;
    case ribi_t::west:	return W_Start;
    default:		return (img_t)-1;
    }
}


/*
 *  member function:
 *      bruecke_besch_t::gib_rampe()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Richtigen Index für Rampenstart ück bestimmen
 *
 *  Return type:
 *      bruecke_besch_t::img_t
 *
 *  Argumente:
 *      ribi_t::ribi ribi
 */
bruecke_besch_t::img_t bruecke_besch_t::gib_rampe(ribi_t::ribi ribi)
{
    switch(ribi) {
    case ribi_t::nord:	return N_Rampe;
    case ribi_t::sued:	return S_Rampe;
    case ribi_t::ost:	return O_Rampe;
    case ribi_t::west:	return W_Rampe;
    default:		return (img_t)-1;
    }
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
