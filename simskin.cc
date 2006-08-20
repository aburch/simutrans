//@ADOC
/////////////////////////////////////////////////////////////////////////////
//
//  simskin.cpp
//
//  (c) 2002 by Volker Meyer, Lohsack 1, D-23843 Lohsack
//
//---------------------------------------------------------------------------
//     Project: sim                          Compiler: MS Visual C++ v6.00
//  SubProject: ...                              Type: C/C++ Source
//  $Workfile:: simskin.cpp          $       $Author: hajo $
//  $Revision: 1.6 $         $Date: 2004/10/30 09:20:48 $
//---------------------------------------------------------------------------
//  Module Description:
//      ...
//
//---------------------------------------------------------------------------
//  Revision History:
//  $Log: simskin.cc,v $
//  Revision 1.6  2004/10/30 09:20:48  hajo
//  sync for Dario
//
//  Revision 1.5  2004/02/03 19:26:11  hajo
//  Hajo: sync for Hendrik
//
//  Revision 1.4  2003/07/23 19:55:52  hajo
//  Hajo: sync for Volker
//
//  Revision 1.3  2003/02/26 09:41:37  hajo
//  Hajo: sync for 0.81.23exp
//
//  Revision 1.2  2003/01/08 19:54:02  hajo
//  Hajo: preparation for delivery to volker
//
//  Revision 1.1  2002/09/18 19:25:52  hajo
//  Volker: new config system
//
//
/////////////////////////////////////////////////////////////////////////////
//@EDOC

/////////////////////////////////////////////////////////////////////////////
//
//  static data
//
/////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "simdebug.h"

#include "besch/skin_besch.h"
#include "besch/spezial_obj_tpl.h"
#include "simskin.h"


#ifdef _MSC_VER
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif

/*
 * Alle Skin-Bestandteile, die wir brauchen
 */
const skin_besch_t *skinverwaltung_t::hauptmenu = NULL;
const skin_besch_t *skinverwaltung_t::schienen_werkzeug = NULL;
const skin_besch_t *skinverwaltung_t::strassen_werkzeug = NULL;
const skin_besch_t *skinverwaltung_t::schiffs_werkzeug = NULL;
const skin_besch_t *skinverwaltung_t::hang_werkzeug = NULL;
const skin_besch_t *skinverwaltung_t::special_werkzeug = NULL;
const skin_besch_t *skinverwaltung_t::listen_werkzeug = NULL;
const skin_besch_t *skinverwaltung_t::edit_werkzeug = NULL;
const skin_besch_t *skinverwaltung_t::farbmenu = NULL;

const skin_besch_t *skinverwaltung_t::logosymbol = NULL;
const skin_besch_t *skinverwaltung_t::neujahrsymbol = NULL;
const skin_besch_t *skinverwaltung_t::neueweltsymbol = NULL;
const skin_besch_t *skinverwaltung_t::flaggensymbol = NULL;
const skin_besch_t *skinverwaltung_t::meldungsymbol = NULL;
const skin_besch_t *skinverwaltung_t::bauigelsymbol = NULL;
const skin_besch_t *skinverwaltung_t::zughaltsymbol = NULL;
const skin_besch_t *skinverwaltung_t::autohaltsymbol = NULL;
const skin_besch_t *skinverwaltung_t::shiffshaltsymbol = NULL;
const skin_besch_t *skinverwaltung_t::bushaltsymbol = NULL;

const skin_besch_t *skinverwaltung_t::fragezeiger = NULL;
const skin_besch_t *skinverwaltung_t::signalzeiger = NULL;
const skin_besch_t *skinverwaltung_t::downzeiger = NULL;
const skin_besch_t *skinverwaltung_t::upzeiger = NULL;
const skin_besch_t *skinverwaltung_t::belegtzeiger = NULL;
const skin_besch_t *skinverwaltung_t::killzeiger = NULL;
const skin_besch_t *skinverwaltung_t::slopezeiger = NULL;
const skin_besch_t *skinverwaltung_t::fahrplanzeiger = NULL;
const skin_besch_t *skinverwaltung_t::strassentunnelzeiger = NULL;
const skin_besch_t *skinverwaltung_t::schienentunnelzeiger = NULL;
const skin_besch_t *skinverwaltung_t::frachthofzeiger = NULL;
const skin_besch_t *skinverwaltung_t::werftNSzeiger = NULL;
const skin_besch_t *skinverwaltung_t::werftOWzeiger = NULL;
const skin_besch_t *skinverwaltung_t::stadtzeiger = NULL;
const skin_besch_t *skinverwaltung_t::baumzeiger = NULL;
const skin_besch_t *skinverwaltung_t::undoc_zeiger = NULL;

const skin_besch_t *skinverwaltung_t::signale = NULL;
const skin_besch_t *skinverwaltung_t::presignals = NULL;
const skin_besch_t *skinverwaltung_t::baugrube = NULL;
const skin_besch_t *skinverwaltung_t::fussweg = NULL;
const skin_besch_t *skinverwaltung_t::pumpe = NULL;
const skin_besch_t *skinverwaltung_t::senke = NULL;

const skin_besch_t *skinverwaltung_t::oberleitung = NULL;
const skin_besch_t *skinverwaltung_t::electricity = NULL;


/**
 * Window skin images
 * @author Hj. Malthaner
 */
const skin_besch_t *skinverwaltung_t::window_skin = NULL;


static spezial_obj_tpl<skin_besch_t> misc_objekte[] = {
    { &skinverwaltung_t::senke,		    "PowerDest" },
    { &skinverwaltung_t::pumpe,		    "PowerSource" },
    { &skinverwaltung_t::signale,	    "Signals" },
    { &skinverwaltung_t::presignals,	    "preSignals" },
    { &skinverwaltung_t::baugrube,	    "Construction" },
    { &skinverwaltung_t::fussweg,	    "Sidewalk" },
    { &skinverwaltung_t::oberleitung,	    "Overheadpower" },
    { NULL, NULL }
};

static spezial_obj_tpl<skin_besch_t> menu_objekte[] = {
    { &skinverwaltung_t::window_skin,	    "WindowSkin" },
    { &skinverwaltung_t::hauptmenu,	    "MainMenu" },
    { &skinverwaltung_t::schienen_werkzeug, "RailTools" },
    { &skinverwaltung_t::strassen_werkzeug, "RoadTools" },
    { &skinverwaltung_t::schiffs_werkzeug,  "ShipTools" },
    { &skinverwaltung_t::hang_werkzeug,     "SlopeTools" },
    { &skinverwaltung_t::special_werkzeug,     "SpecialTools" },
    { &skinverwaltung_t::listen_werkzeug,     "ListTools" },
    { &skinverwaltung_t::farbmenu,	    "ColorMenu" },
    { &skinverwaltung_t::edit_werkzeug,	    "EditTools" },
    { NULL, NULL }
};

static spezial_obj_tpl<skin_besch_t> symbol_objekte[] = {
    { &skinverwaltung_t::logosymbol,	    "Logo" },
    { &skinverwaltung_t::neujahrsymbol,    "NewYear" },
    { &skinverwaltung_t::neueweltsymbol,    "NewWorld" },
    { &skinverwaltung_t::flaggensymbol,	    "Flags" },
    { &skinverwaltung_t::meldungsymbol,	    "Message" },
    { &skinverwaltung_t::bauigelsymbol,	    "Builder" },
    { &skinverwaltung_t::zughaltsymbol,	    "TrainStop" },
    { &skinverwaltung_t::autohaltsymbol,    "CarStop" },
    { &skinverwaltung_t::shiffshaltsymbol,  "ShipStop" },
    { &skinverwaltung_t::bushaltsymbol,	    "BusStop" },
    { &skinverwaltung_t::electricity,	    "Electricity" },
    { NULL, NULL }
};

static spezial_obj_tpl<skin_besch_t> cursor_objekte[] = {
    { &skinverwaltung_t::fragezeiger,		"Query" },
    { &skinverwaltung_t::signalzeiger,		"Signal" },
    { &skinverwaltung_t::downzeiger,		"Down" },
    { &skinverwaltung_t::upzeiger,		"Up" },
    { &skinverwaltung_t::belegtzeiger,		"Marked" },
    { &skinverwaltung_t::killzeiger,		"Remove" },
    { &skinverwaltung_t::slopezeiger,		"Slope" },
    { &skinverwaltung_t::fahrplanzeiger,	"Schedule" },
    { &skinverwaltung_t::strassentunnelzeiger,	"RoadTunnel" },
    { &skinverwaltung_t::schienentunnelzeiger,	"RailTunnel" },
    { &skinverwaltung_t::frachthofzeiger,	"CarStop" },
    { &skinverwaltung_t::werftNSzeiger,		"ShipDepotNS" },
    { &skinverwaltung_t::werftOWzeiger,		"ShipDepotEW" },
    { &skinverwaltung_t::stadtzeiger,		"City" },
    { &skinverwaltung_t::baumzeiger,		"Baum" },
    { &skinverwaltung_t::undoc_zeiger,		"Undocumented" },
    { NULL, NULL }
};


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      skinverwaltung_t::alles_geladen()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Return type:
//      bool
//
//  Arguments:
//      skintyp_t type
/////////////////////////////////////////////////////////////////////////////
//@EDOC
bool skinverwaltung_t::alles_geladen(skintyp_t type)
{
    spezial_obj_tpl<skin_besch_t> *sb;

    switch(type) {
    case menu:
	sb = menu_objekte;
	break;
    case cursor:
	sb = cursor_objekte;
	break;
    case symbol:
	sb = symbol_objekte;
	break;
    case misc:
	sb = misc_objekte + 2;
	break;
    case nothing:
	return true;
    default:
	return false;
    }
    return ::alles_geladen(sb);
}



//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      skinverwaltung_t::register_besch()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Return type:
//      bool
//
//  Arguments:
//      const skin_besch_t *besch
/////////////////////////////////////////////////////////////////////////////
//@EDOC
bool skinverwaltung_t::register_besch(skintyp_t type, const skin_besch_t *besch)
{
    spezial_obj_tpl<skin_besch_t> *sb;

    switch(type) {
    case menu:
	sb = menu_objekte;
	break;
    case cursor:
	sb = cursor_objekte;
	break;
    case symbol:
	sb = symbol_objekte;
	break;
    case misc:
	sb = misc_objekte;
	break;
    case nothing:
	return true;
    default:
	return false;
    }
    return ::register_besch(sb, besch);
}

/////////////////////////////////////////////////////////////////////////////
//@EOF
/////////////////////////////////////////////////////////////////////////////
