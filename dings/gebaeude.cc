/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */
#include <string.h>
#include <ctype.h>
#include "../bauer/hausbauer.h"
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
#include "../besch/intro_dates.h"
#include "../bauer/warenbauer.h"

#include "../besch/grund_besch.h"

#include "../utils/cbuffer_t.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "../gui/stadt_info.h"
#include "../gui/fabrik_info.h"
#include "../gui/money_frame.h"

#include "gebaeude.h"


/**
 * Initializes all variables with save, usable values
 * @author Hj. Malthaner
 */
void gebaeude_t::init()
{
	tile = NULL;
	ptr.fab = 0;
	is_factory = false;
	anim_time = 0;
	sync = false;
	count = 0;
	zeige_baugrube = false;
}



gebaeude_t::gebaeude_t(karte_t *welt) : ding_t(welt)
{
	init();
}



gebaeude_t::gebaeude_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	init();
	rdwr(file);
	if(file->get_version()<88002) {
		setze_yoff(0);
	}
	if(tile  &&  tile->gib_phasen()>1) {
		welt->sync_add( this );
		sync = true;
	}
}



gebaeude_t::gebaeude_t(karte_t *welt, koord3d pos, spieler_t *sp, const haus_tile_besch_t *t) :
    ding_t(welt, pos)
{
	setze_besitzer( sp );

	init();
	if(t) {
		setze_tile(t);	// this will set init time etc.
	}

	grund_t *gr=welt->lookup(pos);
	if(gr  &&  gr->gib_weg_hang()!=gr->gib_grund_hang()) {
		setze_yoff(-TILE_HEIGHT_STEP);
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
	if(get_stadt()) {
		ptr.stadt->remove_gebaeude_from_stadt(this);
	}

	if(tile  &&  tile->gib_phasen()>1  ||  zeige_baugrube) {
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
			dbg->fatal("gebaeude_t::setze_fab()","building already bound to city!");
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
	if(is_factory  &&  ptr.fab!=NULL) {
		dbg->fatal("gebaeude_t::setze_stadt()","building already bound to factory!");
	}
	// sets the pointer in non-zero
	is_factory = false;
	ptr.stadt = s;
}




/* make this building without construction */
void
gebaeude_t::add_alter(uint32 a)
{
	insta_zeit -= min(a,insta_zeit);
}




void
gebaeude_t::setze_tile(const haus_tile_besch_t *new_tile)
{
	insta_zeit = welt->gib_zeit_ms();

	if(!zeige_baugrube  &&  tile!=NULL) {
		// mark old tile dirty
		sint16 ypos = 0;
		for(  int i=0;  i<256;  i++  ) {
			image_id bild = gib_bild(i);
			if(bild==IMG_LEER) {
				break;
			}
			mark_image_dirty( bild, 0 );
			ypos -= get_tile_raster_width();
		}
	}

	zeige_baugrube = !new_tile->gib_besch()->ist_ohne_grube();
	if(sync) {
		if(new_tile->gib_phasen()<=1  &&  !zeige_baugrube) {
			// need to stop animation
			welt->sync_remove(this);
			sync = false;
			count = 0;
		}
	}
	else if(new_tile->gib_phasen()>1  ||  zeige_baugrube) {
		// needs now anymation
		count = simrand(new_tile->gib_phasen());
		anim_time = 0;
		welt->sync_add(this);
		sync = true;
	}
	tile = new_tile;
	set_flag(ding_t::dirty);
}




/**
 * Methode für Echtzeitfunktionen eines Objekts.
 * @return false wenn Objekt aus der Liste der synchronen
 * Objekte entfernt werden sol
 * @author Hj. Malthaner
 */
bool gebaeude_t::sync_step(long delta_t)
{
	if(zeige_baugrube) {
		// still under construction?
		if(welt->gib_zeit_ms() - insta_zeit > 5000) {
			set_flag(ding_t::dirty);
			mark_image_dirty(gib_bild(),0);
			zeige_baugrube = false;
			if(tile->gib_phasen()<=1) {
				welt->sync_remove( this );
				sync = false;
			}
		}
	}
	else {
		// normal animated building
		anim_time += delta_t;
		if(anim_time>tile->gib_besch()->get_animation_time()) {
			if(!zeige_baugrube)  {

				for(int i=1;  ;  i++) {
					image_id bild = gib_bild(i);
					if(bild==IMG_LEER) {
						break;
					}
					mark_image_dirty(bild,-(i<<6));
				}
				// old positions need redraw
				mark_image_dirty(gib_bild(),0);

				anim_time %= tile->gib_besch()->get_animation_time();
				count ++;
				if(count >= tile->gib_phasen()) {
					count = 0;
				}
				// winter for buildings only above snowline
				set_flag(ding_t::dirty);
			}
		}
	}
	return true;
}



void
gebaeude_t::calc_bild()
{
	// need no ground?
	if(!tile->gib_besch()->ist_mit_boden()) {
		grund_t *gr=welt->lookup(gib_pos());
		if(gr  &&  gr->gib_typ()==grund_t::fundament) {
			gr->setze_bild( IMG_LEER );
		}
	}
}



image_id
gebaeude_t::gib_bild() const
{
	if(umgebung_t::hide_buildings!=0) {
		// opaque houses
		if(gib_haustyp()!=unbekannt) {
			return umgebung_t::hide_with_transparency ? skinverwaltung_t::fussweg->gib_bild_nr(0) : skinverwaltung_t::construction_site->gib_bild_nr(0);
		} else if (umgebung_t::hide_buildings == umgebung_t::ALL_HIDDEN_BUIDLING && tile->gib_besch()->gib_utyp() < haus_besch_t::weitere) {
			// special bilding
			if(umgebung_t::hide_with_transparency ) {
				if (tile->gib_besch()->gib_utyp() == haus_besch_t::fabrik && ptr.fab->gib_besch()->gib_platzierung() == fabrik_besch_t::Wasser) {
					// no ground tiles for water thingies
					return IMG_LEER;
				}
				return skinverwaltung_t::fussweg->gib_bild_nr(0);
			}
			else {
				int kind=skinverwaltung_t::construction_site->gib_bild_anzahl()<=tile->gib_besch()->gib_utyp() ? skinverwaltung_t::construction_site->gib_bild_anzahl()-1 : tile->gib_besch()->gib_utyp();
				return skinverwaltung_t::construction_site->gib_bild_nr( kind );
			}
		}
	}

	// winter for buildings only above snowline
	if(zeige_baugrube)  {
		return skinverwaltung_t::construction_site->gib_bild_nr(0);
	}
	else {
		return tile->gib_hintergrund(count, 0, gib_pos().z>=welt->get_snowline());
	}
}



image_id
gebaeude_t::gib_outline_bild() const
{
	if(umgebung_t::hide_buildings!=0  &&  umgebung_t::hide_with_transparency  &&  !zeige_baugrube) {
		// opaque houses
		return tile->gib_hintergrund(count, 0, gib_pos().z>=welt->get_snowline());
	}
	return IMG_LEER;
}



/* gives outline colour and plots background tile if needed for transparent view */
PLAYER_COLOR_VAL
gebaeude_t::gib_outline_colour() const
{
	COLOR_VAL colours[] = { COL_BLACK, COL_YELLOW, COL_YELLOW, COL_PURPLE, COL_RED, COL_GREEN };
	PLAYER_COLOR_VAL disp_colour = 0;
	if(umgebung_t::hide_buildings!=umgebung_t::NOT_HIDE) {
		if(gib_haustyp()!=unbekannt) {
			disp_colour = colours[0] | TRANSPARENT50_FLAG | OUTLINE_FLAG;
		} else if (umgebung_t::hide_buildings == umgebung_t::ALL_HIDDEN_BUIDLING && tile->gib_besch()->gib_utyp() < haus_besch_t::weitere) {
			// special bilding
			disp_colour = colours[tile->gib_besch()->gib_utyp()] | TRANSPARENT50_FLAG | OUTLINE_FLAG;
		}
	}
	return disp_colour;
}



image_id
gebaeude_t::gib_bild(int nr) const
{
	if(zeige_baugrube || umgebung_t::hide_buildings) {
		return IMG_LEER;
	}
	else {
		// winter for buildings only above snowline
		return tile->gib_hintergrund(count, nr, gib_pos().z>=welt->get_snowline());
	}
}



image_id
gebaeude_t::gib_after_bild() const
{
	if(zeige_baugrube) {
		return IMG_LEER;
	}
	if (umgebung_t::hide_buildings != 0 && tile->gib_besch()->gib_utyp() < haus_besch_t::weitere) {
		return IMG_LEER;
	}
	else {
		// Show depots, station buildings etc.
		return tile->gib_vordergrund(count, gib_pos().z>=welt->get_snowline());
	}
}



/*
 * calculate the passenger level as funtion of the city size (if there)
 */
int gebaeude_t::gib_passagier_level() const
{
	koord dim = tile->gib_besch()->gib_groesse();
	long pax = tile->gib_besch()->gib_level();
	if (!is_factory && ptr.stadt != NULL) {
		// belongs to a city ...
		return (((pax+6)>>2)*umgebung_t::passenger_factor)/16;
	}
	return pax*dim.x*dim.y;
}

int gebaeude_t::gib_post_level() const
{
	koord dim = tile->gib_besch()->gib_groesse();
	long post = tile->gib_besch()->gib_post_level();
	if (!is_factory && ptr.stadt != NULL) {
		return (((post+5)>>2)*umgebung_t::passenger_factor)/16;
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
				case haus_besch_t::attraction_city:   return "Besonderes Gebaeude";
				case haus_besch_t::attraction_land:   return "Sehenswuerdigkeit";
				case haus_besch_t::denkmal:           return "Denkmal";
				case haus_besch_t::rathaus:           return "Rathaus";
				default: break;
			}
			break;
	}
	return "Gebaeude";
}

bool gebaeude_t::ist_rathaus() const
{
    return tile->gib_besch()->ist_rathaus();
}

bool gebaeude_t::is_monument() const
{
	return tile->gib_besch()->gib_utyp() == haus_besch_t::denkmal;
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
		return new fabrik_info_t(ptr.fab, this);
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
		grund_t *gr = welt->lookup(gib_pos() - k);
		if(!gr) {
			gr = welt->lookup_kartenboden(gib_pos().gib_2d() - k);
		}
		gebaeude_t* gb = gr->find<gebaeude_t>();
		// is the info of the (0,0) tile on multi tile buildings
		if(gb) {
			// some version made buildings, that had not tile (0,0)!
			gb->zeige_info();
		}
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
				create_win(-1, -1, -1, city->gib_stadt_info(), w_info, magic_none); /* otherwise only on image is allowed */

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
			const char *trans_desc = translator::translate(desc);
			if(trans_desc==desc) {
				// no descrition here
				switch(gib_haustyp()) {
					case wohnung:
						trans_desc = translator::translate("residential house");
						break;
					case industrie:
						trans_desc = translator::translate("industrial building");
						break;
					case gewerbe:
						trans_desc = translator::translate("shops and stores");
						break;
					default:
						// use file name
						break;
				}
				buf.append(trans_desc);
			}
			else {
				// since the format changed, we remove all but double newlines
				char *text = new char[strlen(trans_desc)+1];
				char *dest = text;
				const char *src = trans_desc;
				while(  *src!=0  ) {
					*dest = *src;
					if(src[0]=='\n') {
						if(src[1]=='\n') {
							src ++;
							dest++;
							*dest = '\n';
						}
						else {
							*dest = ' ';
						}
					}
					src ++;
					dest ++;
				}
				*dest = 0;
				trans_desc = text;
				buf.append(trans_desc);
				delete [] text;
			}
			buf.append("\n\n");
		}

		// belongs to which city?
		if (!is_factory && ptr.stadt != NULL) {
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
			buf.printf(translator::translate("Constructed by %s"), maker);
		}
	}
}



void
gebaeude_t::rdwr(loadsave_t *file)
{
	if(!is_factory) {
		// do not save factory buildings => factory will reconstruct them
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
			if(tile==NULL) {
				// try with compatibility list first
				tile = hausbauer_t::find_tile(translator::compatibility_name(buf), idx);
				if(tile==NULL) {
					DBG_MESSAGE("gebaeude_t::rdwr()","neither %s nor %s, tile %i not found, try replacement",translator::compatibility_name(buf),buf,idx);
				}
			}
			if(tile==NULL) {
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
						tile = hausbauer_t::gib_wohnhaus(level,welt->get_timeline_year_month(),welt->get_climate(gib_pos().z))->gib_tile(0);
					} else if(strcmp(typ_str,"COM")==0) {
DBG_MESSAGE("gebaeude_t::rwdr", "replace unknown building %s with commercial level %i",buf,level);
						tile = hausbauer_t::gib_gewerbe(level,welt->get_timeline_year_month(),welt->get_climate(gib_pos().z))->gib_tile(0);
					} else if(strcmp(typ_str,"IND")==0) {
DBG_MESSAGE("gebaeude_t::rwdr", "replace unknown building %s with industrie level %i",buf,level);
						tile = hausbauer_t::gib_industrie(level,welt->get_timeline_year_month(),welt->get_climate(gib_pos().z))->gib_tile(0);
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
			if (tile && tile->gib_besch()->gib_utyp() == haus_besch_t::denkmal) {
				hausbauer_t::denkmal_gebaut(tile->gib_besch());
			}
		}

		if(file->get_version()<99006) {
			// ignore the sync flag
			uint8 dummy=sync;
			file->rdwr_byte(dummy, "\n");
		}

		// restore city pointer here
		if(  file->get_version()>=99014  ) {
			sint32 city_index = -1;
			if(  file->is_saving()  &&  ptr.stadt!=NULL  ) {
				city_index = welt->gib_staedte().index_of( ptr.stadt );
			}
			file->rdwr_long( city_index, "c" );
			if(  file->is_loading()  &&  city_index!=-1  ) {
				ptr.stadt = welt->gib_staedte()[city_index];
			}
		}

		if(file->is_loading()) {
			count = 0;
			anim_time = 0;
			sync = false;

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
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void
gebaeude_t::laden_abschliessen()
{
	// if there is no description (i.e. building not found) we must avoid acessing it => skip calculation
	if(tile) {
		calc_bild();
	}
	else {
		zeige_baugrube = true;
	}
	if(gib_besitzer()) {
		gib_besitzer()->add_maintenance(umgebung_t::maint_building);
	}
	// citybuilding, but no town?
	if(tile->gib_offset()==koord(0,0)  &&  tile->gib_besch()->is_connected_with_town()) {
		stadt_t *city = (ptr.stadt==NULL) ? welt->suche_naechste_stadt( gib_pos().gib_2d() ) : ptr.stadt;
		if(city) {
			city->add_gebaeude_to_stadt(this);
		}
	}
}



void gebaeude_t::entferne(spieler_t *sp)
{
//	DBG_MESSAGE("gebaeude_t::entferne()","gb %i");
	// remove costs
	if(sp) {
		if(gib_haustyp()!=unbekannt  ||  tile->gib_besch()->gib_utyp()<haus_besch_t::bahnhof) {
			sp->buche(umgebung_t::cst_multiply_remove_haus*(tile->gib_besch()->gib_level()+1), gib_pos().gib_2d(), COST_CONSTRUCTION);
		}
		else {
			// tearing down halts is always single costs only
			sp->buche(umgebung_t::cst_multiply_remove_haus, gib_pos().gib_2d(), COST_CONSTRUCTION);
		}
	}


	// may need to update next buildings, in the case of start, middle, end buildings
	if(tile->gib_besch()->gib_all_layouts()>1  &&  gib_haustyp()==unbekannt) {

		// realign surrounding buildings...
		uint32 layout = tile->gib_layout();

		// detect if we are connected at far (north/west) end
		grund_t * gr = welt->lookup( gib_pos() );
		if(gr) {
			sint8 offset = gr->gib_weg_yoff()/TILE_HEIGHT_STEP;
			gr = welt->lookup( gib_pos()+koord3d( (layout & 1 ? koord::ost : koord::sued), offset) );
			if(!gr) {
				// check whether bridge end tile
				grund_t * gr_tmp = welt->lookup( gib_pos()+koord3d( (layout & 1 ? koord::ost : koord::sued),offset - 1) );
				if(gr_tmp && gr_tmp->gib_weg_yoff()/TILE_HEIGHT_STEP == 1) {
					gr = gr_tmp;
				}
			}
			if(gr) {
				gebaeude_t* gb = gr->find<gebaeude_t>();
				if(gb  &&  gb->gib_tile()->gib_besch()->gib_all_layouts()>4) {
					koord xy = gb->gib_tile()->gib_offset();
					uint8 layoutbase = gb->gib_tile()->gib_layout();
					if((layoutbase & 1) == (layout & 1)) {
						layoutbase |= 4; // set far bit on neighbour
						gb->setze_tile(gb->gib_tile()->gib_besch()->gib_tile(layoutbase, xy.x, xy.y));
					}
				}
			}

			// detect if near (south/east) end
			gr = welt->lookup( gib_pos()+koord3d( (layout & 1 ? koord::west : koord::nord), offset) );
			if(!gr) {
				// check whether bridge end tile
				grund_t * gr_tmp = welt->lookup( gib_pos()+koord3d( (layout & 1 ? koord::west : koord::nord),offset - 1) );
				if(gr_tmp && gr_tmp->gib_weg_yoff()/TILE_HEIGHT_STEP == 1) {
					gr = gr_tmp;
				}
			}
			if(gr) {
				gebaeude_t* gb = gr->find<gebaeude_t>();
				if(gb  &&  gb->gib_tile()->gib_besch()->gib_all_layouts()>4) {
					koord xy = gb->gib_tile()->gib_offset();
					uint8 layoutbase = gb->gib_tile()->gib_layout();
					if((layoutbase & 1) == (layout & 1)) {
						layoutbase |= 2; // set near bit on neighbour
						gb->setze_tile(gb->gib_tile()->gib_besch()->gib_tile(layoutbase, xy.x, xy.y));
					}
				}
			}
		}
	}
}
