/*
 * hausanim.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */
#include <string.h>
#include <ctype.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simfab.h"
#include "../simimg.h"
#include "../simhalt.h"
#include "../simwin.h"
#include "../simcity.h"
#include "../simplay.h"
#include "../simmem.h"
#include "../simtools.h"
#include "../simdebug.h"
#include "../simintr.h"
#include "../simskin.h"

#include "../boden/grund.h"


#include "../besch/haus_besch.h"
#include "../bauer/warenbauer.h"

#include "../utils/cbuffer_t.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/freelist.h"

#include "../gui/stadt_info.h"
#include "../gui/fabrik_info.h"
#include "../gui/money_frame.h"

#include "gebaeude.h"


uint8 gebaeude_t::hide = NOT_HIDDEN;



/**
 * Initializes all variables with save, usable values
 * @author Hj. Malthaner
 */
void gebaeude_t::init(const haus_tile_besch_t *t)
{
	tile = t;
	ptr.fab = 0;
	is_factory = true;
	anim_time = 0;
	sync = false;
	count = 0;
	zeige_baugrube = t ? !t->gib_besch()->ist_ohne_grube() : false;
}


gebaeude_t::gebaeude_t(karte_t *welt) : ding_t(welt)
{
    init(0);
}


gebaeude_t::gebaeude_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	init(0);
	rdwr(file);
	if(file->get_version()<88002) {
		setze_yoff(0);
	}
	if(gib_besitzer()) {
		gib_besitzer()->add_maintenance(umgebung_t::maint_building);
	}
}

gebaeude_t::gebaeude_t(karte_t *welt, koord3d pos,spieler_t *sp, const haus_tile_besch_t *t) :
    ding_t(welt, pos)
{
	setze_besitzer( sp );

	tile = t;
	renoviere();
	init(t);

	grund_t *gr=welt->lookup(pos);
	if(gr!=0  &&  gr->gib_weg_hang()!=gr->gib_grund_hang()) {
		setze_yoff(-16);
	}

	if(gib_besitzer()) {
		gib_besitzer()->add_maintenance(umgebung_t::maint_building);
	}
}

/**
 * Destructor. Removes this from the list of sync objects if neccesary.
 *
 * @author Hj. Malthaner
 */
gebaeude_t::~gebaeude_t()
{
	if(sync) {
		sync = false;
		welt->sync_remove(this);
	}

	// tiles might be invalid, if no description is found during loading
	if(tile  &&  tile->gib_besch()  &&  tile->gib_besch()->ist_ausflugsziel()) {
		welt->remove_ausflugsziel(this);
	}

	count = 0;
	anim_time = 0;
	if(gib_besitzer()) {
		gib_besitzer()->add_maintenance(-umgebung_t::maint_building);
	}

#ifdef DEBUG
	// to detect access to invalid tiles
	setze_pos( koord3d::invalid );
#endif
}



/* sets the corresponding pointer to a factory
 * @author prissi
 */
void
gebaeude_t::setze_fab(fabrik_t *fb)
{
	// sets the pointer in non-zero
	if(fb) {
		if(!is_factory  &&  ptr.stadt!=NULL) {
			dbg->error("gebaeude_t::setze_fab()","building already bound to city!");
		}
		is_factory = true;
		ptr.fab = fb;
	}
	else if(is_factory) {
		ptr.fab = NULL;
	}
}



/* sets the corresponding city
 * @author prissi
 */
void
gebaeude_t::setze_stadt(stadt_t *s)
{
	// sets the pointer in non-zero
	if(s) {
		if(is_factory  &&  ptr.fab!=NULL) {
			dbg->error("gebaeude_t::setze_stadt()","building already bound to factory!");
		}
		is_factory = false;
		ptr.stadt = s;
	}
	else if(!is_factory) {
		is_factory = true;
		ptr.stadt = NULL;
	}
}



/* must be defined here, otherwise tile might be unknown
 */
void
gebaeude_t::setze_count(uint8 count)
{
	this->count = count % tile->gib_phasen();
}



/* make this building without construction */
void
gebaeude_t::add_alter(uint32 a)
{
	insta_zeit -= min(a,insta_zeit);
	zeige_baugrube = false;
	if(is_factory==false  ||  ptr.fab==NULL) {
		step_frequency = 0;
	}
	set_flag(ding_t::dirty);
//	DBG_MESSAGE("gebaeude_t::add_alter()","");
}




void
gebaeude_t::renoviere()
{
    insta_zeit = welt->gib_zeit_ms();
    zeige_baugrube = true;
    step_frequency = 1;
}




/**
 * Should only be called after everything is set up to play
 * the animation actually. Sets sync flag and register/deregisters
 * this as a sync object
 *
 * @author Hj. Malthaner
 */
void
gebaeude_t::setze_sync(bool yesno)
{
	if(yesno) {
		// already sync? and worth animating ?
		if(!sync && tile->gib_phasen()>1) {
			// no
			sync = true;
			anim_time = simrand(300);
			count = simrand(tile->gib_phasen());
			welt->sync_add(this);
		}
	}
	else {
		sync = false;
		// always deregister ... doesn't hurt if we were not registered.
		welt->sync_remove(this);
	}
}



bool
gebaeude_t::step(long delta_t)
{
	// still under construction?
	if(zeige_baugrube) {
		if(welt->gib_zeit_ms() - insta_zeit > 5000) {
			zeige_baugrube = false;
			set_flag(ding_t::dirty);
			// factories needs more frequent steps
			if(is_factory  &&  ptr.fab   &&  ptr.fab->gib_pos()==gib_pos()) {
				step_frequency = 1;
			}
			else {
				step_frequency = 0;
			}
			// need no ground?
			if(!tile->gib_besch()->ist_mit_boden()) {
				grund_t *gr=welt->lookup(gib_pos());
				if(gr  &&  gr->gib_typ()==grund_t::fundament) {
					gr->setze_bild( IMG_LEER );
				}
			}
		}
	}

	// factory produces
	if(is_factory  &&  ptr.fab!=NULL) {
		ptr.fab->step(delta_t);
		INT_CHECK("gebaeude 250");
	}

	return true;
}



/* image calculation routines
 */
image_id
gebaeude_t::gib_bild() const
{
	if(hide!=NOT_HIDDEN) {
		if(gib_haustyp()!=unbekannt) {
			return skinverwaltung_t::construction_site->gib_bild_nr(0);
		}
		else if(hide==ALL_HIDDEN  &&  tile->gib_besch()->gib_utyp()<=hausbauer_t::weitere) {
			// special bilding
			int kind=skinverwaltung_t::construction_site->gib_bild_anzahl()<=tile->gib_besch()->gib_utyp() ? skinverwaltung_t::construction_site->gib_bild_anzahl()-1 : tile->gib_besch()->gib_utyp();
			return skinverwaltung_t::construction_site->gib_bild_nr( kind );
		}
	}

	if(zeige_baugrube)  {
		return skinverwaltung_t::construction_site->gib_bild_nr(0);
	}
	else {
		return tile->gib_hintergrund(count, 0);
	}
}

image_id
gebaeude_t::gib_bild(int nr) const
{
	if(zeige_baugrube || hide) {
		return IMG_LEER;
	}
	else {
		return tile->gib_hintergrund(count, nr);
	}
}

image_id
gebaeude_t::gib_after_bild() const
{
	if(zeige_baugrube) {
		return IMG_LEER;
	}
	else {
		return tile->gib_vordergrund(count, 0);
	}
}

image_id
gebaeude_t::gib_after_bild(int nr) const
{
	if(zeige_baugrube) {
		return IMG_LEER;
	}
	else {
		return tile->gib_vordergrund(count, nr);
	}
}



/* calculate the passenger level as funtion of the city size (if there)
 */
int gebaeude_t::gib_passagier_level() const
{
	koord dim = tile->gib_besch()->gib_groesse();
	long pax = tile->gib_besch()->gib_level();
	if(is_factory==false  &&  ptr.stadt!=NULL) {
		// belongs to a city ...
#if 0
		// old density
		const koord lo(ptr.stadt->get_linksoben()), ru(ptr.stadt->get_rechtsunten());
		const long dense=(lo.x-ru.x)*(lo.y-ru.y);
		return (2097*((pax+6)>>2))/dense;
#else
		return (((pax+6)>>2)*umgebung_t::passenger_factor)/16;
#endif
	}
	return pax*dim.x*dim.y;
}

int gebaeude_t::gib_post_level() const
{
	koord dim = tile->gib_besch()->gib_groesse();
	long post = tile->gib_besch()->gib_post_level();
	if(is_factory==false  &&  ptr.stadt!=NULL) {
#if 0
		// old density
		const koord lo(ptr.stadt->get_linksoben()), ru(ptr.stadt->get_rechtsunten());
		const long dense=(lo.x-ru.x)*(lo.y-ru.y);
		return (2097*((post+5)>>2))/dense;
#else
		return (((post+5)>>2)*umgebung_t::passenger_factor)/16;
#endif
	}
	return post*dim.x*dim.y;
}



/**
 * @return eigener Name oder Name der Fabrik falls Teil einer Fabrik
 * @author Hj. Malthaner
 */
const char *gebaeude_t::gib_name() const
{
	if(is_factory  &&  ptr.fab) {
		return ptr.fab->gib_name();
	}
    switch(tile->gib_besch()->gib_typ()) {
    case wohnung:
	break;//return "Wohnhaus";
    case gewerbe:
	break;//return "Gewerbehaus";
    case industrie:
	break;//return "Industriegebäude";
    default:
	switch(tile->gib_besch()->gib_utyp()) {
	case hausbauer_t::special:
	    return "Besonderes Gebaeude";
	case hausbauer_t::sehenswuerdigkeit:
	    return "Sehenswuerdigkeit";
	case hausbauer_t::denkmal:
	    return "Denkmal";
	case hausbauer_t::rathaus:
	    return "Rathaus";
	default:
	    break;
	}
    }
    return "Gebaeude";
}

bool gebaeude_t::ist_rathaus() const
{
    return tile->gib_besch()->ist_rathaus();
}

bool gebaeude_t::is_monument() const
{
    return tile->gib_besch()->gib_utyp()==hausbauer_t::denkmal;
}

bool gebaeude_t::ist_firmensitz() const
{
    return tile->gib_besch()->ist_firmensitz();
}

gebaeude_t::typ gebaeude_t::gib_haustyp() const
{
    return tile->gib_besch()->gib_typ();
}



ding_infowin_t *gebaeude_t::new_info()
{
	if (is_factory  &&  ptr.fab) {
		return new fabrik_info_t(ptr.fab, this, welt);
	}
	else {
		return ding_t::new_info();
	}
}


void
gebaeude_t::zeige_info()
{
	// Für die Anzeige ist bei mehrteiliggen Gebäuden immer
	// das erste laut Layoutreihenfolge zuständig.
	// Sonst gibt es für eine 2x2-Fabrik bis zu 4 Infofenster.
	koord k = tile->gib_offset();
	if(k != koord(0, 0)) {
		ding_t *gb = welt->lookup(gib_pos() - k)->obj_bei(0);

		if(gb)
			gb->zeige_info();	// an den ersten deligieren
		}
		else {
DBG_MESSAGE("gebaeude_t::zeige_info()", "at %d,%d - name is '%s'", gib_pos().x, gib_pos().y, gib_name());

				if(ist_firmensitz()) {
					if(umgebung_t::townhall_info) {
						ding_t::zeige_info();
					}
			         create_win(-1, -1, -1, gib_besitzer()->gib_money_frame(), w_info);
				}

			if(!tile->gib_besch()->ist_ohne_info()) {

				if(ist_rathaus()) {
				stadt_t *city = welt->suche_naechste_stadt(gib_pos().gib_2d());
				create_win(-1, -1, -1,
					city->gib_stadt_info(),
					w_info,
					magic_none); /* otherwise only on image is allowed */

				if(umgebung_t::townhall_info) {
					ding_t::zeige_info();
				}
			}
			else {
				ding_t::zeige_info();
			}
		}
	}
}



void gebaeude_t::info(cbuffer_t & buf) const
{
	ding_t::info(buf);

	if(is_factory  &&  ptr.fab != NULL) {
		ptr.fab->info(buf);
	}
	else if(zeige_baugrube) {
		buf.append(translator::translate("Baustelle"));
		buf.append("\n");
	}
	else {
		const char *desc = tile->gib_besch()->gib_name();
		if(desc != NULL) {
			buf.append(translator::translate(desc));
			buf.append("\n");
		}

		// belongs to which city?
		if(is_factory==false  &&  ptr.stadt!=NULL) {
			static char buffer[256];
			sprintf(buffer,translator::translate("Town: %s\n"),ptr.stadt->gib_name());
			buf.append(buffer);
		}

		buf.append(translator::translate("Passagierrate"));
		buf.append(": ");
		buf.append(gib_passagier_level());
		buf.append("\n");

		buf.append(translator::translate("Postrate"));
		buf.append(": ");
		buf.append(gib_post_level());
		buf.append("\n");

		buf.append(translator::translate("\nBauzeit von"));
		buf.append(tile->gib_besch()->get_intro_year_month()/12);
		if(tile->gib_besch()->get_retire_year_month()!=DEFAULT_RETIRE_DATE*12) {
			buf.append(translator::translate("\nBauzeit bis"));
			buf.append(tile->gib_besch()->get_retire_year_month()/12);
		}

		buf.append("\n");
		if(gib_besitzer()==NULL) {
			buf.append("\n");
			buf.append(translator::translate("Wert"));
			buf.append(": ");
			buf.append(-umgebung_t::cst_multiply_remove_haus*(tile->gib_besch()->gib_level()+1)/100);
			buf.append("$\n");
		}

		const char *maker=tile->gib_besch()->gib_copyright();
		if(maker!=NULL  && maker[0]!=0) {
			buf.append("\n");
			buf.append(translator::translate("Constructed by"));
			buf.append(maker);
		}
	}
}


void
gebaeude_t::rdwr(loadsave_t *file)
{
	if(get_fabrik()==NULL || file->is_loading()) {
		// Gebaude, die zu einer Fabrik gehoeren werden beim laden
		// der Fabrik neu erzeugt
		ding_t::rdwr(file);

		char  buf[256];
		short idx;

		if(file->is_saving()) {
			const char *s = tile->gib_besch()->gib_name();
			idx = tile->gib_index();
			file->rdwr_str(s, "N");
		}
		else {
			file->rd_str_into(buf, "N");
		}

		file->rdwr_short(idx, "\n");
		file->rdwr_long(insta_zeit, " ");

		if(file->is_loading()) {
			tile = hausbauer_t::find_tile(buf, idx);

			if(!tile) {
				// first check for special buildings
				if(strstr(buf,"TrainStop")!=NULL) {
					tile = hausbauer_t::find_tile("TrainStop", idx);
				} else if(strstr(buf,"BusStop")!=NULL) {
					tile = hausbauer_t::find_tile("BusStop", idx);
				} else if(strstr(buf,"ShipStop")!=NULL) {
					tile = hausbauer_t::find_tile("ShipStop", idx);
				} else if(strstr(buf,"PostOffice")!=NULL) {
					tile = hausbauer_t::find_tile("PostOffice", idx);
				} else if(strstr(buf,"StationBlg")!=NULL) {
					tile = hausbauer_t::find_tile("StationBlg", idx);
				}
				else {
					// try to find a fitting building
					char typ_str[16];
					int i, level=atoi(buf);

					if(level>0) {
						// May be an old 64er, so we can try some
						if(strncmp(buf+3,"WOHN",4)==0) {
							strcpy(typ_str,"RES");
						} else if(strncmp(buf+3,"FAB",3)==0) {
							strcpy(typ_str,"IND");
						} else {
							strcpy(typ_str,"COM");
						}
					}
					else {
						sscanf(buf,"%3s_%i_%i",typ_str,&i,&level);
						typ_str[0] = toupper(typ_str[0]);
						typ_str[1] = toupper(typ_str[1]);
						typ_str[2] = toupper(typ_str[2]);
					}
					//		DBG_MESSAGE("gebaeude_t::rwdr", "%s level %i",typ_str,level);
					if(strcmp(typ_str,"RES")==0) {
DBG_MESSAGE("gebaeude_t::rwdr", "replace unknown building %s with residence level %i",buf,level);
						tile = hausbauer_t::gib_wohnhaus(level,welt->get_timeline_year_month())->gib_tile(0);
					} else if(strcmp(typ_str,"COM")==0) {
DBG_MESSAGE("gebaeude_t::rwdr", "replace unknown building %s with commercial level %i",buf,level);
						tile = hausbauer_t::gib_gewerbe(level,welt->get_timeline_year_month())->gib_tile(0);
					} else if(strcmp(typ_str,"IND")==0) {
DBG_MESSAGE("gebaeude_t::rwdr", "replace unknown building %s with industrie level %i",buf,level);
						tile = hausbauer_t::gib_industrie(level,welt->get_timeline_year_month())->gib_tile(0);
					} else {
DBG_MESSAGE("gebaeude_t::rwdr", "description %s for building at %d,%d not found (will be removed)!", buf, gib_pos().x, gib_pos().y);
					}
				}
			}	// here we should have a valid tile pointer or nothing ...


			// Denkmäler sollen nicht doppelt gebaut werden, daher
			// checken wir hier über den Gebäudenamen, ob wir ein Denkmal sind.
			// Sollte später mal hübscher gehen: entweder speichern
			// der gebauten denkmäler
			// oder aber eine feste Zuordnung von gebaeude_alt_t
			// und Beschreibung.
			if(tile && tile->gib_besch()->gib_utyp() == hausbauer_t::denkmal) {
				hausbauer_t::denkmal_gebaut(tile->gib_besch());
			}
		}

		uint8 dummy=sync;
		file->rdwr_byte(dummy, "\n");
		sync = dummy;

		if(file->is_loading()) {
			count = 0;
			anim_time = 0;
			if(tile && sync) {
				// Sicherstellen, dass alles wieder animiert wird!
				// Ohne "sync=false" denkt setze_sync(), es dreht sich
				// schon alles.
				sync = false;
				setze_sync(true);
			}

			// Hajo: rebuild tourist attraction list
			if(tile && tile->gib_besch()->ist_ausflugsziel()) {
				welt->add_ausflugsziel( this );
			}
		}
	}
	else {
		file->wr_obj_id(-1);
	}
}



/**
 * Methode für Echtzeitfunktionen eines Objekts.
 * @return false wenn Objekt aus der Liste der synchronen
 * Objekte entfernt werden sol
 * @author Hj. Malthaner
 */
bool gebaeude_t::sync_step(long delta_t)
{
// DBG_MESSAGE("gebaeude_t::sync_step()", "%p, %d phases", this, phasen);

	anim_time += delta_t;
	if(anim_time > 300) {
		anim_time = 0;
		count ++;
		if(count >= tile->gib_phasen()) {
			count = 0;
		}
		set_flag(ding_t::dirty);
	}
	return true;
}



void
gebaeude_t::entferne(spieler_t *sp)
{
//	DBG_MESSAGE("gebaeude_t::entferne()","gb %i");
	if(!is_factory  &&  ptr.stadt!=NULL) {
		ptr.stadt->remove_gebaeude_from_stadt(this);
	}

	if(sp) {
		sp->buche(umgebung_t::cst_multiply_remove_haus*(tile->gib_besch()->gib_level()+1), gib_pos().gib_2d(), COST_CONSTRUCTION);
	}
}
