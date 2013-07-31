#include "besch/skin_besch.h"
#include "besch/spezial_obj_tpl.h"
#include "simskin.h"

#include "tpl/slist_tpl.h"
/*
 * Alle Skin-Bestandteile, die wir brauchen
 */

 // colours
COLOR_VAL skinverwaltung_t::theme_color_highlight           = MN_GREY4;
COLOR_VAL skinverwaltung_t::theme_color_shadow              = MN_GREY0;
COLOR_VAL skinverwaltung_t::theme_color_face                = MN_GREY2;
COLOR_VAL skinverwaltung_t::theme_color_button_text         = COL_BLACK;
COLOR_VAL skinverwaltung_t::theme_color_text                = COL_WHITE;
COLOR_VAL skinverwaltung_t::theme_color_selected_text       = COL_WHITE;
COLOR_VAL skinverwaltung_t::theme_color_selected_background = COL_BLUE;
COLOR_VAL skinverwaltung_t::theme_color_static_text         = COL_BLACK;
COLOR_VAL skinverwaltung_t::theme_color_disabled_text       = MN_GREY4;

// menus
const skin_besch_t* skinverwaltung_t::werkzeuge_general  = NULL;
const skin_besch_t* skinverwaltung_t::werkzeuge_simple   = NULL;
const skin_besch_t* skinverwaltung_t::werkzeuge_dialoge  = NULL;
const skin_besch_t* skinverwaltung_t::werkzeuge_toolbars = NULL;

/* Window skin images are menus too! */
const skin_besch_t* skinverwaltung_t::window_skin = NULL;

// symbol images
const skin_besch_t* skinverwaltung_t::biglogosymbol      = NULL;
const skin_besch_t* skinverwaltung_t::logosymbol         = NULL;
const skin_besch_t* skinverwaltung_t::neujahrsymbol      = NULL;
const skin_besch_t* skinverwaltung_t::neueweltsymbol     = NULL;
const skin_besch_t* skinverwaltung_t::flaggensymbol      = NULL;
const skin_besch_t* skinverwaltung_t::meldungsymbol      = NULL;
const skin_besch_t* skinverwaltung_t::zughaltsymbol      = NULL;
const skin_besch_t* skinverwaltung_t::autohaltsymbol     = NULL;
const skin_besch_t* skinverwaltung_t::schiffshaltsymbol  = NULL;
const skin_besch_t* skinverwaltung_t::airhaltsymbol      = NULL;
const skin_besch_t* skinverwaltung_t::monorailhaltsymbol = NULL;
const skin_besch_t* skinverwaltung_t::maglevhaltsymbol   = NULL;
const skin_besch_t* skinverwaltung_t::narrowgaugehaltsymbol = NULL;
const skin_besch_t* skinverwaltung_t::bushaltsymbol      = NULL;
const skin_besch_t* skinverwaltung_t::tramhaltsymbol     = NULL;
const skin_besch_t* skinverwaltung_t::networksymbol      = NULL;
const skin_besch_t* skinverwaltung_t::timelinesymbol     = NULL;
const skin_besch_t* skinverwaltung_t::fastforwardsymbol  = NULL;
const skin_besch_t* skinverwaltung_t::pausesymbol        = NULL;

const skin_besch_t* skinverwaltung_t::electricity        = NULL;
const skin_besch_t* skinverwaltung_t::intown             = NULL;
const skin_besch_t* skinverwaltung_t::passagiere         = NULL;
const skin_besch_t* skinverwaltung_t::post               = NULL;
const skin_besch_t* skinverwaltung_t::waren              = NULL;
const skin_besch_t* skinverwaltung_t::station_type       = NULL;
const skin_besch_t* skinverwaltung_t::seasons_icons      = NULL;
const skin_besch_t* skinverwaltung_t::message_options    = NULL;
const skin_besch_t* skinverwaltung_t::color_options      = NULL;

// cursors
const skin_besch_t* skinverwaltung_t::cursor_general     = NULL;	// new cursors
const skin_besch_t* skinverwaltung_t::bauigelsymbol      = NULL;
const skin_besch_t* skinverwaltung_t::belegtzeiger       = NULL;
const skin_besch_t* skinverwaltung_t::mouse_cursor       = NULL;

// misc images
const skin_besch_t* skinverwaltung_t::construction_site  = NULL;
const skin_besch_t* skinverwaltung_t::fussweg            = NULL;
const skin_besch_t* skinverwaltung_t::pumpe              = NULL;
const skin_besch_t* skinverwaltung_t::senke              = NULL;
const skin_besch_t* skinverwaltung_t::tunnel_texture     = NULL;

slist_tpl<const skin_besch_t *>skinverwaltung_t::extra_obj;


static spezial_obj_tpl<skin_besch_t> misc_objekte[] = {
	{ &skinverwaltung_t::senke,             "PowerDest"    },
	{ &skinverwaltung_t::pumpe,             "PowerSource"  },
	{ &skinverwaltung_t::construction_site, "Construction" },
	{ &skinverwaltung_t::fussweg,           "Sidewalk"     },
	{ &skinverwaltung_t::tunnel_texture,    "TunnelTexture"},
	{ NULL, NULL }
};

static spezial_obj_tpl<skin_besch_t> menu_objekte[] = {
	// new menu system
	{ &skinverwaltung_t::window_skin,       "WindowSkin"   },
	{ &skinverwaltung_t::werkzeuge_general, "GeneralTools" },
	{ &skinverwaltung_t::werkzeuge_simple,  "SimpleTools"  },
	{ &skinverwaltung_t::werkzeuge_dialoge, "DialogeTools" },
	{ &skinverwaltung_t::werkzeuge_toolbars,"BarTools"     },
	{ NULL, NULL }
};

static spezial_obj_tpl<skin_besch_t> symbol_objekte[] = {
	{ &skinverwaltung_t::seasons_icons,      "Seasons"        },
	{ &skinverwaltung_t::message_options,    "MessageOptions" },
	{ &skinverwaltung_t::color_options,      "ColorOptions"   },
	{ &skinverwaltung_t::logosymbol,         "Logo"           },
	{ &skinverwaltung_t::neujahrsymbol,      "NewYear"        },
	{ &skinverwaltung_t::neueweltsymbol,     "NewWorld"       },
	{ &skinverwaltung_t::flaggensymbol,      "Flags"          },
	{ &skinverwaltung_t::meldungsymbol,      "Message"        },
	{ &skinverwaltung_t::electricity,        "Electricity"    },
	{ &skinverwaltung_t::intown,             "InTown"         },
	{ &skinverwaltung_t::passagiere,         "Passagiere"     },
	{ &skinverwaltung_t::post,               "Post"           },
	{ &skinverwaltung_t::waren,              "Waren"          },
	{ NULL, NULL }
};

// simutrans will work without those
static spezial_obj_tpl<skin_besch_t> fakultative_objekte[] = {
	{ &skinverwaltung_t::biglogosymbol,      "BigLogo"        },
	{ &skinverwaltung_t::mouse_cursor,       "Mouse"          },
	{ &skinverwaltung_t::zughaltsymbol,      "TrainStop"      },
	{ &skinverwaltung_t::autohaltsymbol,     "CarStop"        },
	{ &skinverwaltung_t::schiffshaltsymbol,  "ShipStop"       },
	{ &skinverwaltung_t::bushaltsymbol,      "BusStop"        },
	{ &skinverwaltung_t::airhaltsymbol,      "AirStop"        },
	{ &skinverwaltung_t::monorailhaltsymbol, "MonorailStop"   },
	{ &skinverwaltung_t::maglevhaltsymbol,   "MaglevStop"     },
	{ &skinverwaltung_t::narrowgaugehaltsymbol,"NarrowgaugeStop"},
	{ &skinverwaltung_t::tramhaltsymbol,     "TramStop"       },
	{ &skinverwaltung_t::networksymbol,      "networksym"     },
	{ &skinverwaltung_t::timelinesymbol,     "timelinesym"    },
	{ &skinverwaltung_t::fastforwardsymbol,  "fastforwardsym" },
	{ &skinverwaltung_t::pausesymbol,        "pausesym"       },
	{ &skinverwaltung_t::station_type,       "station_type"   },
	{ NULL, NULL }
};

static spezial_obj_tpl<skin_besch_t> cursor_objekte[] = {
	// old cursors
	{ &skinverwaltung_t::bauigelsymbol,  "Builder"      },
	{ &skinverwaltung_t::cursor_general, "GeneralTools" },
	{ &skinverwaltung_t::belegtzeiger,   "Marked"       },
	{ NULL, NULL }
};


bool skinverwaltung_t::alles_geladen(skintyp_t type)
{
	spezial_obj_tpl<skin_besch_t>* sb;
	switch (type) {
		case menu:    sb = menu_objekte+1;       break;
		case cursor:  sb = cursor_objekte;     break;
		case symbol:  sb = symbol_objekte;     break;
		case misc:
			sb = misc_objekte+2;
			// for compatibility: use sidewalk as tunneltexture
			if (tunnel_texture==NULL) {
				tunnel_texture = fussweg;
			}
			break;
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
	if(  !::register_besch(sb, besch)  ) {
		// currently no misc objects allowed ...
		if(  !(type==cursor  ||  type==symbol)  ) {
			if(  type==menu  ) {
				extra_obj.insert( besch );
				dbg->message( "skinverwaltung_t::register_besch()","Extra object %s added.", besch->get_name() );
			}
			else {
				dbg->warning("skinverwaltung_t::register_besch()","Spurious object '%s' loaded (will not be referenced anyway)!", besch->get_name() );
			}
		}
		else {
			return ::register_besch( fakultative_objekte,  besch );
		}
	}
	return true;
}



// return the extra_obj with this name
const skin_besch_t *skinverwaltung_t::get_extra( const char *str, int len )
{
	FOR(slist_tpl<skin_besch_t const*>, const s, skinverwaltung_t::extra_obj) {
		if (strncmp(str, s->get_name(), len) == 0) {
			return s;
		}
	}
	return NULL;
}
