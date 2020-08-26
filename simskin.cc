/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "descriptor/skin_desc.h"
#include "descriptor/spezial_obj_tpl.h"
#include "simskin.h"

#include "tpl/slist_tpl.h"
/*
 * Alle Skin-Bestandteile, die wir brauchen
 */

// menus
const skin_desc_t* skinverwaltung_t::tool_icons_general  = NULL;
const skin_desc_t* skinverwaltung_t::tool_icons_simple   = NULL;
const skin_desc_t* skinverwaltung_t::tool_icons_dialoge = NULL;
const skin_desc_t* skinverwaltung_t::tool_icons_toolbars = NULL;
const skin_desc_t* skinverwaltung_t::toolbar_background = NULL;

/* Window skin images are menus too! */
const skin_desc_t* skinverwaltung_t::button = NULL;
const skin_desc_t* skinverwaltung_t::round_button = NULL;
const skin_desc_t* skinverwaltung_t::check_button = NULL;
const skin_desc_t* skinverwaltung_t::posbutton = NULL;
const skin_desc_t* skinverwaltung_t::back = NULL;
const skin_desc_t* skinverwaltung_t::scrollbar = NULL;
const skin_desc_t* skinverwaltung_t::divider = NULL;
const skin_desc_t* skinverwaltung_t::editfield = NULL;
const skin_desc_t* skinverwaltung_t::listbox = NULL;
const skin_desc_t* skinverwaltung_t::gadget= NULL;

// symbol images
const skin_desc_t* skinverwaltung_t::biglogosymbol      = NULL;
const skin_desc_t* skinverwaltung_t::logosymbol         = NULL;
const skin_desc_t* skinverwaltung_t::neujahrsymbol      = NULL;
const skin_desc_t* skinverwaltung_t::neueweltsymbol     = NULL;
const skin_desc_t* skinverwaltung_t::flaggensymbol      = NULL;
const skin_desc_t* skinverwaltung_t::meldungsymbol      = NULL;
const skin_desc_t* skinverwaltung_t::zughaltsymbol      = NULL;
const skin_desc_t* skinverwaltung_t::autohaltsymbol     = NULL;
const skin_desc_t* skinverwaltung_t::schiffshaltsymbol  = NULL;
const skin_desc_t* skinverwaltung_t::airhaltsymbol      = NULL;
const skin_desc_t* skinverwaltung_t::monorailhaltsymbol = NULL;
const skin_desc_t* skinverwaltung_t::maglevhaltsymbol   = NULL;
const skin_desc_t* skinverwaltung_t::narrowgaugehaltsymbol = NULL;
const skin_desc_t* skinverwaltung_t::bushaltsymbol      = NULL;
const skin_desc_t* skinverwaltung_t::tramhaltsymbol     = NULL;
const skin_desc_t* skinverwaltung_t::networksymbol      = NULL;
const skin_desc_t* skinverwaltung_t::timelinesymbol     = NULL;
const skin_desc_t* skinverwaltung_t::fastforwardsymbol  = NULL;
const skin_desc_t* skinverwaltung_t::pausesymbol        = NULL;

const skin_desc_t* skinverwaltung_t::electricity        = NULL;
const skin_desc_t* skinverwaltung_t::intown             = NULL;
const skin_desc_t* skinverwaltung_t::upgradable         = NULL;
const skin_desc_t* skinverwaltung_t::missing_scheduled_slot = NULL;
const skin_desc_t* skinverwaltung_t::passengers         = NULL;
const skin_desc_t* skinverwaltung_t::mail               = NULL;
const skin_desc_t* skinverwaltung_t::goods              = NULL;
const skin_desc_t* skinverwaltung_t::goods_categories   = NULL;
const skin_desc_t* skinverwaltung_t::station_type       = NULL;
const skin_desc_t* skinverwaltung_t::seasons_icons      = NULL;
const skin_desc_t* skinverwaltung_t::message_options    = NULL;
const skin_desc_t* skinverwaltung_t::color_options      = NULL;

const skin_desc_t* skinverwaltung_t::compass_iso        = NULL;
const skin_desc_t* skinverwaltung_t::compass_rect       = NULL;	// compass rectangular (for minimap)

const skin_desc_t* skinverwaltung_t::pax_evaluation_icons = NULL;
const skin_desc_t* skinverwaltung_t::mail_evaluation_icons = NULL;

const skin_desc_t* skinverwaltung_t::alerts             = NULL;
const skin_desc_t* skinverwaltung_t::input_output       = NULL; // industry input/output
const skin_desc_t* skinverwaltung_t::travel_time        = NULL; // travel time / lead time
const skin_desc_t* skinverwaltung_t::in_transit         = NULL; // goods in transit
const skin_desc_t* skinverwaltung_t::ind_sector_symbol  = NULL;
const skin_desc_t* skinverwaltung_t::reverse_arrows     = NULL;
const skin_desc_t* skinverwaltung_t::waiting_time       = NULL; // waiting time at the station
const skin_desc_t* skinverwaltung_t::service_frequency  = NULL; // line service frequency
const skin_desc_t* skinverwaltung_t::on_foot            = NULL;

// cursors
const skin_desc_t* skinverwaltung_t::cursor_general     = NULL;	// new cursors
const skin_desc_t* skinverwaltung_t::bauigelsymbol      = NULL;
const skin_desc_t* skinverwaltung_t::belegtzeiger       = NULL;
const skin_desc_t* skinverwaltung_t::mouse_cursor       = NULL;

// misc images
const skin_desc_t* skinverwaltung_t::construction_site  = NULL;
const skin_desc_t* skinverwaltung_t::fussweg            = NULL;
const skin_desc_t* skinverwaltung_t::pumpe              = NULL;
const skin_desc_t* skinverwaltung_t::senke              = NULL;
const skin_desc_t* skinverwaltung_t::tunnel_texture     = NULL;
const skin_desc_t* skinverwaltung_t::ribi_arrow         = NULL;

slist_tpl<const skin_desc_t *>skinverwaltung_t::extra_obj;


static spezial_obj_tpl<skin_desc_t> const misc_objekte[] = {
	{ &skinverwaltung_t::ribi_arrow,        "RibiArrow"    },
	{ &skinverwaltung_t::senke,             "PowerDest"    },
	{ &skinverwaltung_t::pumpe,             "PowerSource"  },
	{ &skinverwaltung_t::construction_site, "Construction" },
	{ &skinverwaltung_t::fussweg,           "Sidewalk"     },
	{ &skinverwaltung_t::tunnel_texture,    "TunnelTexture"},
	{ NULL, NULL }
};

static spezial_obj_tpl<skin_desc_t> const menu_objekte[] = {
	// new menu system
	{ &skinverwaltung_t::button,            "Button"   },
	{ &skinverwaltung_t::round_button,      "Roundbutton"  },
	{ &skinverwaltung_t::check_button,      "Checkbutton"  },
	{ &skinverwaltung_t::posbutton,         "Posbutton"    },
	{ &skinverwaltung_t::scrollbar,         "Scrollbar"    },
	{ &skinverwaltung_t::divider,           "Divider"      },
	{ &skinverwaltung_t::editfield,         "Editfield"    },
	{ &skinverwaltung_t::listbox,           "Listbox"      },
	{ &skinverwaltung_t::back,              "Back"         },
	{ &skinverwaltung_t::gadget,            "Gadget"       },
	{ &skinverwaltung_t::tool_icons_general, "GeneralTools" },
	{ &skinverwaltung_t::tool_icons_simple,  "SimpleTools"  },
	{ &skinverwaltung_t::tool_icons_dialoge, "DialogeTools" },
	{ &skinverwaltung_t::tool_icons_toolbars,"BarTools"     },
	{ NULL, NULL }
};

static spezial_obj_tpl<skin_desc_t> const symbol_objekte[] = {
	{ &skinverwaltung_t::missing_scheduled_slot, "MissingScheduledSlot" },
	{ &skinverwaltung_t::upgradable,         "Upgradable"     },
	{ &skinverwaltung_t::pax_evaluation_icons, "PassengersEvaluation" },
	{ &skinverwaltung_t::mail_evaluation_icons, "MailEvaluation" },
	{ &skinverwaltung_t::alerts,             "Alerts"         },
	{ &skinverwaltung_t::goods_categories,   "GoodsCategories"},
	{ &skinverwaltung_t::input_output,       "InputOutput"    },
	{ &skinverwaltung_t::travel_time,        "TravelTime"     },
	{ &skinverwaltung_t::in_transit,         "InTransit"      },
	{ &skinverwaltung_t::ind_sector_symbol,  "IndustrySectors" },
	{ &skinverwaltung_t::reverse_arrows,     "ReverseArrows"  },
	{ &skinverwaltung_t::waiting_time,       "WaitingTime"    },
	{ &skinverwaltung_t::service_frequency,  "ServiceFrequency" },
	{ &skinverwaltung_t::on_foot,            "OnFoot"         },
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
	{ &skinverwaltung_t::passengers,         "Passagiere"     },
	{ &skinverwaltung_t::mail,               "Post"           },
	{ &skinverwaltung_t::goods,              "Waren"          },
	{ NULL, NULL }
};

// simutrans will work without those
static spezial_obj_tpl<skin_desc_t> const fakultative_objekte[] = {
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
	{ &skinverwaltung_t::toolbar_background,"ToolsBackground"},
	{ &skinverwaltung_t::compass_iso,        "CompassIso"     },
	{ &skinverwaltung_t::compass_rect,       "CompassRect"    },
	{ NULL, NULL }
};

static spezial_obj_tpl<skin_desc_t> const cursor_objekte[] = {
	// old cursors
	{ &skinverwaltung_t::bauigelsymbol,  "Builder"      },
	{ &skinverwaltung_t::cursor_general, "GeneralTools" },
	{ &skinverwaltung_t::belegtzeiger,   "Marked"       },
	{ NULL, NULL }
};


bool skinverwaltung_t::successfully_loaded(skintyp_t type)
{
	spezial_obj_tpl<skin_desc_t> const* sb;
	switch (type) {
		case menu:    sb = menu_objekte+1;     break;
		case cursor:  sb = cursor_objekte;     break;
		case symbol:  sb = symbol_objekte+14;   break;
		case misc:
			sb = misc_objekte+3;
			// for compatibility: use sidewalk as tunneltexture
			if (tunnel_texture==NULL) {
				tunnel_texture = fussweg;
			}
			break;
		case nothing: return true;
		default:      return false;
	}
	return ::successfully_loaded(sb);
}


bool skinverwaltung_t::register_desc(skintyp_t type, const skin_desc_t* desc)
{
	spezial_obj_tpl<skin_desc_t> const* sb;
	switch (type) {
		case menu:    sb = menu_objekte;   break;
		case cursor:  sb = cursor_objekte; break;
		case symbol:  sb = symbol_objekte; break;
		case misc:    sb = misc_objekte;   break;
		case nothing: return true;
		default:      return false;
	}
	if(  !::register_desc(sb, desc)  ) {
		// currently no misc objects allowed ...
		if(  !(type==cursor  ||  type==symbol)  ) {
			if(  type==menu  ) {
				extra_obj.insert( desc );
				dbg->message( "skinverwaltung_t::register_desc()","Extra object %s added.", desc->get_name() );
			}
			else {
				dbg->warning("skinverwaltung_t::register_desc()","Spurious object '%s' loaded (will not be referenced anyway)!", desc->get_name() );
			}
		}
		else {
			return ::register_desc( fakultative_objekte,  desc );
		}
	}
	return true;
}



// return the extra_obj with this name
const skin_desc_t *skinverwaltung_t::get_extra( const char *str, int len )
{
	FOR(slist_tpl<skin_desc_t const*>, const s, skinverwaltung_t::extra_obj) {
		if (strncmp(str, s->get_name(), len) == 0) {
			return s;
		}
	}
	return NULL;
}
