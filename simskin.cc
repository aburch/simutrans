#include "besch/skin_besch.h"
#include "besch/spezial_obj_tpl.h"
#include "simskin.h"

/*
 * Alle Skin-Bestandteile, die wir brauchen
 */
const skin_besch_t* skinverwaltung_t::hauptmenu          = NULL;
const skin_besch_t* skinverwaltung_t::schiffs_werkzeug   = NULL;
const skin_besch_t* skinverwaltung_t::hang_werkzeug      = NULL;
const skin_besch_t* skinverwaltung_t::special_werkzeug   = NULL;
const skin_besch_t* skinverwaltung_t::listen_werkzeug    = NULL;
const skin_besch_t* skinverwaltung_t::edit_werkzeug      = NULL;
const skin_besch_t* skinverwaltung_t::farbmenu           = NULL;

const skin_besch_t* skinverwaltung_t::biglogosymbol      = NULL;
const skin_besch_t* skinverwaltung_t::logosymbol         = NULL;
const skin_besch_t* skinverwaltung_t::neujahrsymbol      = NULL;
const skin_besch_t* skinverwaltung_t::neueweltsymbol     = NULL;
const skin_besch_t* skinverwaltung_t::flaggensymbol      = NULL;
const skin_besch_t* skinverwaltung_t::meldungsymbol      = NULL;
const skin_besch_t* skinverwaltung_t::bauigelsymbol      = NULL;
const skin_besch_t* skinverwaltung_t::zughaltsymbol      = NULL;
const skin_besch_t* skinverwaltung_t::autohaltsymbol     = NULL;
const skin_besch_t* skinverwaltung_t::schiffshaltsymbol  = NULL;
const skin_besch_t* skinverwaltung_t::airhaltsymbol      = NULL;
const skin_besch_t* skinverwaltung_t::monorailhaltsymbol = NULL;
const skin_besch_t* skinverwaltung_t::bushaltsymbol      = NULL;
const skin_besch_t* skinverwaltung_t::tramhaltsymbol      = NULL;

const skin_besch_t* skinverwaltung_t::fragezeiger        = NULL;
const skin_besch_t* skinverwaltung_t::signalzeiger       = NULL;
const skin_besch_t* skinverwaltung_t::downzeiger         = NULL;
const skin_besch_t* skinverwaltung_t::upzeiger           = NULL;
const skin_besch_t* skinverwaltung_t::belegtzeiger       = NULL;
const skin_besch_t* skinverwaltung_t::killzeiger         = NULL;
const skin_besch_t* skinverwaltung_t::slopezeiger        = NULL;
const skin_besch_t* skinverwaltung_t::fahrplanzeiger     = NULL;
const skin_besch_t* skinverwaltung_t::werftNSzeiger      = NULL;
const skin_besch_t* skinverwaltung_t::werftOWzeiger      = NULL;
const skin_besch_t* skinverwaltung_t::stadtzeiger        = NULL;
const skin_besch_t* skinverwaltung_t::baumzeiger         = NULL;
const skin_besch_t* skinverwaltung_t::undoc_zeiger       = NULL;
const skin_besch_t* skinverwaltung_t::mouse_cursor       = NULL;

const skin_besch_t* skinverwaltung_t::construction_site  = NULL;
const skin_besch_t* skinverwaltung_t::fussweg            = NULL;
const skin_besch_t* skinverwaltung_t::pumpe              = NULL;
const skin_besch_t* skinverwaltung_t::senke              = NULL;

const skin_besch_t* skinverwaltung_t::electricity        = NULL;
const skin_besch_t* skinverwaltung_t::intown             = NULL;
const skin_besch_t* skinverwaltung_t::passagiere         = NULL;
const skin_besch_t* skinverwaltung_t::post               = NULL;
const skin_besch_t* skinverwaltung_t::waren              = NULL;

const skin_besch_t* skinverwaltung_t::seasons_icons      = NULL;

const skin_besch_t* skinverwaltung_t::message_options    = NULL;

/**
 * Window skin images
 * @author Hj. Malthaner
 */
const skin_besch_t* skinverwaltung_t::window_skin = NULL;


static spezial_obj_tpl<skin_besch_t> misc_objekte[] = {
	{ &skinverwaltung_t::senke,             "PowerDest"    },
	{ &skinverwaltung_t::pumpe,             "PowerSource"  },
	{ &skinverwaltung_t::construction_site, "Construction" },
	{ &skinverwaltung_t::fussweg,           "Sidewalk"     },
	{ NULL, NULL }
};

static spezial_obj_tpl<skin_besch_t> menu_objekte[] = {
	{ &skinverwaltung_t::window_skin,      "WindowSkin"   },
	{ &skinverwaltung_t::hauptmenu,        "MainMenu"     },
	{ &skinverwaltung_t::schiffs_werkzeug, "ShipTools"    },
	{ &skinverwaltung_t::hang_werkzeug,    "SlopeTools"   },
	{ &skinverwaltung_t::special_werkzeug, "SpecialTools" },
	{ &skinverwaltung_t::listen_werkzeug,  "ListTools"    },
	{ &skinverwaltung_t::farbmenu,         "ColorMenu"    },
	{ &skinverwaltung_t::edit_werkzeug,    "EditTools"    },
	{ NULL, NULL }
};

static spezial_obj_tpl<skin_besch_t> symbol_objekte[] = {
	{ &skinverwaltung_t::biglogosymbol,      "BigLogo"        },
	{ &skinverwaltung_t::seasons_icons,      "Seasons"        },
	{ &skinverwaltung_t::message_options,    "MessageOptions" },
	{ &skinverwaltung_t::logosymbol,         "Logo"           },
	{ &skinverwaltung_t::neujahrsymbol,      "NewYear"        },
	{ &skinverwaltung_t::neueweltsymbol,     "NewWorld"       },
	{ &skinverwaltung_t::flaggensymbol,      "Flags"          },
	{ &skinverwaltung_t::meldungsymbol,      "Message"        },
	{ &skinverwaltung_t::bauigelsymbol,      "Builder"        },
	{ &skinverwaltung_t::zughaltsymbol,      "TrainStop"      },
	{ &skinverwaltung_t::autohaltsymbol,     "CarStop"        },
	{ &skinverwaltung_t::schiffshaltsymbol,  "ShipStop"       },
	{ &skinverwaltung_t::bushaltsymbol,      "BusStop"        },
	{ &skinverwaltung_t::airhaltsymbol,      "AirStop"        },
	{ &skinverwaltung_t::monorailhaltsymbol, "MonorailStop"   },
	{ &skinverwaltung_t::tramhaltsymbol,     "TramStop"   },
	{ &skinverwaltung_t::electricity,        "Electricity"    },
	{ &skinverwaltung_t::intown,             "InTown"         },
	{ &skinverwaltung_t::passagiere,         "Passagiere"     },
	{ &skinverwaltung_t::post,               "Post"           },
	{ &skinverwaltung_t::waren,              "Waren"          },
	{ NULL, NULL }
};

static spezial_obj_tpl<skin_besch_t> cursor_objekte[] = {
	{ &skinverwaltung_t::mouse_cursor,   "Mouse"        },
	{ &skinverwaltung_t::fragezeiger,    "Query"        },
	{ &skinverwaltung_t::signalzeiger,   "Signal"       },
	{ &skinverwaltung_t::downzeiger,     "Down"         },
	{ &skinverwaltung_t::upzeiger,       "Up"           },
	{ &skinverwaltung_t::belegtzeiger,   "Marked"       },
	{ &skinverwaltung_t::killzeiger,     "Remove"       },
	{ &skinverwaltung_t::slopezeiger,    "Slope"        },
	{ &skinverwaltung_t::fahrplanzeiger, "Schedule"     },
	{ &skinverwaltung_t::werftNSzeiger,  "ShipDepotNS"  },
	{ &skinverwaltung_t::werftOWzeiger,  "ShipDepotEW"  },
	{ &skinverwaltung_t::stadtzeiger,    "City"         },
	{ &skinverwaltung_t::baumzeiger,     "Baum"         },
	{ &skinverwaltung_t::undoc_zeiger,   "Undocumented" },
	{ NULL, NULL }
};


bool skinverwaltung_t::alles_geladen(skintyp_t type)
{
	spezial_obj_tpl<skin_besch_t>* sb;
	switch (type) {
		case menu:    sb = menu_objekte;               break;
		case cursor:  sb = cursor_objekte + 1;         break; // forget about mouse cursor
		case symbol:  sb = symbol_objekte + 1 + 1 + 1; break; // forget about message box options, BigLogo, and seasons
		case misc:    sb = misc_objekte;	             break; // not all signals needed
		case nothing: return true;
		default:      return false;
	}
	return ::alles_geladen(sb);
}


bool skinverwaltung_t::register_besch(skintyp_t type, const skin_besch_t* besch)
{
	spezial_obj_tpl<skin_besch_t>* sb;
	switch (type) {
		case menu:    sb = menu_objekte;   break;
		case cursor:  sb = cursor_objekte; break;
		case symbol:  sb = symbol_objekte; break;
		case misc:    sb = misc_objekte;   break;
		case nothing: return true;
		default:      return false;
	}
	return ::register_besch(sb, besch);
}
