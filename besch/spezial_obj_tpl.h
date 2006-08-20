//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  spezial_obj_tpl.h
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Header
//  $Workfile:: spezial_obj_tpl.h    $       $Author: hajo $
//  $Revision: 1.1 $         $Date: 2002/09/18 19:13:21 $
//---------------------------------------------------------------------------
//  Module Description:
//      Hilfstemplates, um Beschreibungen, die das Programm direkt benötigt,
//      zu initialisieren.
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: spezial_obj_tpl.h,v $
//  Revision 1.1  2002/09/18 19:13:21  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC
#ifndef __SPEZIAL_OBJ_TPL_H
#define __SPEZIAL_OBJ_TPL_H


#include <string.h>
#include <typeinfo>
#include "../tpl/debug_helper.h"

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  struct:
//      template <class besch_t> spezial_obj_tpl()
//
//---------------------------------------------------------------------------
//  Description:
//      Beschreibung eines erforderlichen Objekts. Für die nachfolgenden
//      Funktionen wird eine Liste dieser Beschreibungen angelegt, wobei die
//      Liste mit einem "{NULL, NULL}" Eintrag terminiert wird.
/////////////////////////////////////////////////////////////////////////////
//@EDOC
template <class besch_t> struct spezial_obj_tpl {
    const besch_t **besch;
    const char *name;
};

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  function:
//      register_besch()
//
//---------------------------------------------------------------------------
//  Description:
//      Ein Objektzeiger wird anhand der übergebenen Liste gesetzt, falls der
//      Name des Objektes einem der in der Liste erwähnten Objekte gehört.
//
//  Return type:
//      bool
//
//  Arguments:
//      spezial_obj_tpl<besch_t> *so
//      const besch_t *besch
/////////////////////////////////////////////////////////////////////////////
//@EDOC
template <class besch_t> bool register_besch(spezial_obj_tpl<besch_t> *so,
					     const besch_t *besch)
{
    while(so->name) {
	if(!strcmp(so->name, besch->gib_name())) {
	    *so->besch = besch;
	    return true;
	}
	so++;
    }
    return false;
}

//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  function:
//      alles_geladen()
//
//---------------------------------------------------------------------------
//  Description:
//      Überprüft die übergebene Liste, ob alle Objekte ungleich NULL, sprich
//      geladen sind.
//
//  Return type:
//      bool
//
//  Arguments:
//      spezial_obj_tpl<besch_t> *so
/////////////////////////////////////////////////////////////////////////////
//@EDOC
template <class besch_t> bool alles_geladen(spezial_obj_tpl<besch_t> *so)
{
    while(so->name) {
	if(!*so->besch) {
	    ERROR("alles_geladen()", "%s-object %s not found.\n*** PLEASE INSTALL PROPER BASE FILE AND CHECK PATH ***",
		typeid(**so->besch).name(), so->name);
	    return false;
	}
	so++;
    }
    return true;
}


template <class besch_t> void warne_ungeladene(spezial_obj_tpl<besch_t> *so, int count)
{
    while(count-- && so->name) {
	if(!*so->besch) {
	    dbg->message("warne_ungeladene", "Object %s not found, feature disabled", so->name);
	}
	so++;
    }
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
#endif // __SPEZIAL_OBJ_TPL_H
