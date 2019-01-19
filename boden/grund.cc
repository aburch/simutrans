/*
 * Base class for grounds in simutrans.
 * by Hj. Malthaner
 */

#include <string.h>

#include "../simcolor.h"
#include "../simconst.h"
#include "../simdebug.h"
#include "../simdepot.h"
#include "../simsignalbox.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simhalt.h"
#include "../display/simimg.h"
#include "../player/simplay.h"
#include "../gui/simwin.h"
#include "../simworld.h"

#include "../bauer/wegbauer.h"
#include "../bauer/vehikelbauer.h" /* for diversion route checking */

#include "../descriptor/ground_desc.h"
#include "../descriptor/building_desc.h"
#include "../descriptor/crossing_desc.h"
#include "../descriptor/tunnel_desc.h"
#include "../descriptor/way_desc.h"

#include "../dataobj/freelist.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../obj/baum.h"
#include "../obj/crossing.h"
#include "../obj/groundobj.h"
#include "../obj/label.h"
#include "../obj/leitung2.h"	// for construction of new ways ...
#include "../obj/roadsign.h"
#include "../obj/signal.h"
#include "../obj/tunnel.h"
#include "../obj/wayobj.h"

#include "../gui/ground_info.h"
#include "../gui/karte.h"

#include "../tpl/inthashtable_tpl.h"

#include "../utils/cbuffer_t.h"
# include "../utils/simstring.h"

#include "../vehicle/simpeople.h"

#include "wege/kanal.h"
#include "wege/maglev.h"
#include "wege/monorail.h"
#include "wege/narrowgauge.h"
#include "wege/runway.h"
#include "wege/schiene.h"
#include "wege/strasse.h"
#include "wege/weg.h"

#include "fundament.h"
#include "grund.h"
#include "tunnelboden.h"
#include "wasser.h"


/**
 * Pointer to the world of this ground. Static to conserve space.
 * Change to instance variable once more than one world is available.
 * @author Hj. Malthaner
 */
karte_ptr_t grund_t::welt;
volatile bool grund_t::show_grid = false;

uint8 grund_t::offsets[4]={0,1,2/*illegal!*/,2};

sint8 grund_t::underground_level = 127;
uint8 grund_t::underground_mode = ugm_none;


// ---------------------- text handling from here ----------------------


/**
 * Table of ground texts
 * @author Hj. Malthaner
 */
static inthashtable_tpl<uint64, char*> ground_texts;

#define get_ground_text_key(k) ( (uint64)(k).x + ((uint64)(k).y << 16) + ((uint64)(k).z << 32) )

// and the reverse operation
#define get_ground_koord3d_key(key) koord3d( (key) & 0x00007FFF, ((key)>>16) & 0x00007fff, (key)>>32 )

void grund_t::set_text(const char *text)
{
	if (text==NULL  &&  !get_flag(has_text)) {
		// no text to delete
		return;
	}
	const uint64 n = get_ground_text_key(pos);
	if(  text  ) {
		char *new_text = strdup(text);
		free(ground_texts.remove(n));
		ground_texts.put(n, new_text);
		set_flag(has_text);
	}
	else if(  get_flag(has_text)  ) {
		char *txt=ground_texts.remove(n);
		free(txt);
		clear_flag(has_text);
	}
	set_flag(dirty);
	welt->set_dirty();
}


const char *grund_t::get_text() const
{
	const char *result = 0;
	if(  get_flag(has_text)  ) {
		result = ground_texts.get( get_ground_text_key(pos) );
		if(result==NULL) {
			return "undef";
		}
		assert(result);
	}
	return result;
}


PLAYER_COLOR_VAL grund_t::text_farbe() const
{
	// if this ground belongs to a halt, the color should reflect the halt owner, not the ground owner!
	// Now, we use the color of label_t owner
	if(is_halt()  &&  find<label_t>()==NULL) {
		// only halt label
		const halthandle_t halt = get_halt();
		const player_t *player=halt->get_owner();
		if(player) {
			return PLAYER_FLAG|(player->get_player_color1()+4);
		}
	}
	// else color according to current owner
	else if(obj_bei(0)) {
		const player_t *player = obj_bei(0)->get_owner(); // for cityhall
		const label_t* l = find<label_t>();
		if(l) {
			player = l->get_owner();
		}
		if(player) {
			return PLAYER_FLAG|(player->get_player_color1()+4);
		}
	}

	return SYSCOL_TEXT_HIGHLIGHT;
}


// ---------------- init, rdwr, and destruct from here ---------------------


void* grund_t::operator new(size_t s)
{
	return freelist_t::gimme_node(s);
}


void grund_t::operator delete(void* p, size_t s)
{
	return freelist_t::putback_node(s, p);
}


grund_t::grund_t(loadsave_t *file)
{
	flags = 0;
	back_imageid = 0;
	rdwr(file);
}


void grund_t::rdwr(loadsave_t *file)
{
	koord k = pos.get_2d();

	// water saves its correct height => no need to save grid heights anymore
	sint8 z = welt->lookup_hgt( k ); // save grid height for water tiles - including partial water tiles
	sint8 z_w = welt->get_water_hgt( k );
	if(  !(get_typ() == grund_t::boden  ||  get_typ() == grund_t::wasser) || pos.z > z_w || z > z_w  ) {
		z = pos.z; // all other tiles save ground height
	}

	planquadrat_t *plan = welt->access( k );
	uint8 climate_data = plan->get_climate() + (plan->get_climate_corners() << 4);

	xml_tag_t g( file, "grund_t" );
	if(file->get_version()<101000) {
		pos.rdwr(file);
		z_w = welt->get_groundwater();
	}
	else if(  file->get_version() < 112007  ) {
		file->rdwr_byte(z);
		pos.z = get_typ() == grund_t::wasser ? welt->get_groundwater() : z;
		z_w = welt->get_groundwater();
	}
	else {
		file->rdwr_byte(z);
		file->rdwr_byte(z_w);
		if(  file->is_loading()  &&  !is_water()  &&  !welt->lookup_kartenboden( k )  &&  z < z_w  ) {
			// partially in water, restore correct ground height while keeping grid height
			// if kartenboden doesn't exist we will become it
			pos.z = z_w;
		}
		else if(  file->is_loading()  ) {
			pos.z = get_typ() == grund_t::wasser ? z_w : z;
		}
		file->rdwr_byte(climate_data);
		plan->set_climate((climate)(climate_data & 7));
		plan->set_climate_corners((climate_data >> 4));
	}

	if(  file->is_loading()  &&  file->get_version() < 112007  ) {
		// convert heights from old single height saved game - water already at correct height
		pos.z = get_typ() == grund_t::wasser ? pos.z : pos.z * env_t::pak_height_conversion_factor;
		z = z * env_t::pak_height_conversion_factor;
	}

	if(file->is_saving()) {
		const char *text = get_text();
		file->rdwr_str(text);
	}
	else {
		const char *text = 0;
		file->rdwr_str(text);
		if(text) {
			set_text(text);
			guarded_free(const_cast<char *>(text));
		}
	}

	if(file->get_version()<99007) {
		bool label;
		file->rdwr_bool(label);
		if(label) {
			objlist.add( new label_t(pos, welt->get_player(0), get_text() ) );
		}
	}

	sint8 owner_n=-1;
	if(file->get_version()<99005) {
		file->rdwr_byte(owner_n);
	}

	if(file->get_version()>=88009) {
		uint8 sl = slope;
		if(  file->get_version() < 112007  &&  file->is_saving()  ) {
			// truncate double slopes to single slopes, better than nothing
			sl = min( corner_sw(slope), 1 ) + min( corner_se(slope), 1 ) * 2 + min( corner_ne(slope), 1 ) * 4 + min( corner_nw(slope), 1 ) * 8;
		}
		file->rdwr_byte(sl);
		if(  file->is_loading()  ) {
			slope = sl;
		}
	}
	else {
		// safe init for old version
		slope = 0;
	}

	if(  file->is_loading()  ) {
		if(  file->get_version() < 112007  ) {
			// convert slopes from old single height saved game
			slope = (scorner_sw(slope) + scorner_se(slope) * 3 + scorner_ne(slope) * 9 + scorner_nw(slope) * 27) * env_t::pak_height_conversion_factor;
		}
		if(  !ground_desc_t::double_grounds  ) {
			// truncate double slopes to single slopes
			slope = min( corner_sw(slope), 1 ) + min( corner_se(slope), 1 ) * 3 + min( corner_ne(slope), 1 ) * 9 + min( corner_nw(slope), 1 ) * 27;
		}
	}

	// restore grid
	if(  file->is_loading()  ) {
		// for south/east map edges we need to restore more than one point
		if(  pos.x == welt->get_size().x-1  &&  pos.y == welt->get_size().y-1  ) {
			sint8 z_southeast = z;
			if(  get_typ() == grund_t::wasser  &&  z_southeast > z_w  ) {
				z_southeast = z_w;
			}
			else {
				z_southeast += corner_se(slope);
			}
			welt->set_grid_hgt( k + koord(1,1), z_southeast );
		}
		if(  pos.x == welt->get_size().x-1  ) {
			sint8 z_east = z;
			if(  get_typ() == grund_t::wasser  &&  z_east > z_w  ) {
				z_east = z_w;
			}
			else {
				z_east += corner_ne(slope);
			}
			welt->set_grid_hgt( k + koord(1,0), z_east );
		}
		if(  pos.y == welt->get_size().y-1  ) {
			sint8 z_south = z;
			if(  get_typ() == grund_t::wasser  &&  z_south > z_w  ) {
				z_south = z_w;
			}
			else {
				z_south += corner_sw(slope);
			}
			welt->set_grid_hgt( k + koord(0,1), z_south );
		}

		if(  get_typ() == grund_t::wasser  &&  z > z_w  ) {
			z = z_w;
		}
		else {
			z += corner_nw(slope);
		}
		welt->set_grid_hgt( k, z );
		welt->set_water_hgt( k, z_w );
	}

	// loading ways from here on
	if(file->is_loading()) {
		waytype_t wtyp;
		int i = -1;
		do {
			wtyp = (waytype_t)file->rd_obj_id();
			weg_t *weg = NULL;

			if(++i < 2) {
				switch(wtyp) {
					default:
#if MSG_LEVEL
						if(  wtyp != invalid_wt  ) {
							dbg->error( "grund_t::rdwr()", "invalid waytype %i!", (int)wtyp );
							wtyp = invalid_wt;
						}
#endif
						break;

					case road_wt:
						weg = new strasse_t(file);
						break;

					case monorail_wt:
						weg = new monorail_t(file);
						break;

					case maglev_wt:
						weg = new maglev_t(file);
						break;

					case narrowgauge_wt:
						weg = new narrowgauge_t(file);
						break;

					case track_wt: {
						schiene_t *sch = new schiene_t(file);
						if(sch->get_desc()->get_wtyp()==monorail_wt) {
							dbg->warning("grund_t::rdwr()", "converting railroad to monorail at (%i,%i)",get_pos().x, get_pos().y);
							// compatibility code: Convert to monorail
							monorail_t *w= new monorail_t();
							w->set_desc(sch->get_desc(), true);
							w->set_max_speed(sch->get_max_speed());
							w->set_ribi(sch->get_ribi_unmasked());
							weg->set_max_axle_load(sch->get_max_axle_load());
							weg->set_bridge_weight_limit(sch->get_bridge_weight_limit());
							weg->add_way_constraints(sch->get_way_constraints());
							delete sch;
							weg = w;
						}
						else {
							weg = sch;
						}
					} break;

					case tram_wt:
						weg = new schiene_t(file);
						if(weg->get_desc()->get_styp()!=type_tram) {
							weg->set_desc(way_builder_t::weg_search(tram_wt,weg->get_max_speed(),0,type_tram), true);
						}
						break;

					case water_wt:
						// ignore old type dock ...
						if(file->get_version()>=87000) {
							weg = new kanal_t(file);
						}
						else {
							uint8 d8;
							sint16 d16;
							sint32 d32;

							file->rdwr_byte(d8);
							file->rdwr_short(d16);
							file->rdwr_long(d32);
							file->rdwr_long(d32);
							file->rdwr_long(d32);
							file->rdwr_long(d32);
							DBG_MESSAGE("grund_t::rdwr()","at (%i,%i) dock ignored",get_pos().x, get_pos().y);
						}
						break;

					case air_wt:
						weg = new runway_t(file);
						break;
				}

				if(weg) {
					if(get_typ()==fundament) {
						// remove this (but we can not correct the other ways, since possibly not yet loaded)
						dbg->error("grund_t::rdwr()","removing way from foundation at %i,%i",pos.x,pos.y);
						// we do not delete them, to keep maintenance costs correct
					}
					else {
						assert((flags&has_way2)==0);	// maximum two ways on one tile ...
						weg->set_pos(pos);
						if(owner_n!=-1) {
							weg->set_owner(welt->get_player(owner_n));
						}
						objlist.add(weg);
						if(flags&has_way1) {
							flags |= has_way2;
						}
						flags |= has_way1;
					}
				}
			}
		} while(wtyp != invalid_wt);

		flags |= dirty;
	}
	else {
		// saving all ways ...
		if (weg_t* const w = get_weg_nr(0)) {
			file->wr_obj_id(w->get_waytype());
			w->rdwr(file);
		}
		if (weg_t* const w = get_weg_nr(1)) {
			file->wr_obj_id(w->get_waytype());
			w->rdwr(file);
		}
		file->wr_obj_id(-1);   // Way end
	}

	// all objects on this tile
	objlist.rdwr(file, get_pos());

	// need to add a crossing for old games ...
	if (file->is_loading()  &&  ist_uebergang()  &&  !find<crossing_t>(2)) {
		const crossing_desc_t *cr_desc = crossing_logic_t::get_crossing( ((weg_t *)obj_bei(0))->get_waytype(), ((weg_t *)obj_bei(1))->get_waytype(), ((weg_t *)obj_bei(0))->get_max_speed(), ((weg_t *)obj_bei(1))->get_max_speed(), 0 );
		if(cr_desc==0) {
			dbg->fatal("crossing_t::crossing_t()","requested for waytypes %i and %i but nothing defined!", ((weg_t *)obj_bei(0))->get_waytype(), ((weg_t *)obj_bei(1))->get_waytype() );
		}
		crossing_t *cr = new crossing_t(obj_bei(0)->get_owner(), pos, cr_desc, ribi_t::is_straight_ns(get_weg(cr_desc->get_waytype(1))->get_ribi_unmasked()) );
		objlist.add( cr );
		crossing_logic_t::add( cr, crossing_logic_t::CROSSING_INVALID );
	}
}


grund_t::grund_t(koord3d pos)
{
	this->pos = pos;
	flags = 0;
	set_image(IMG_EMPTY);    // set   flags = dirty;
	back_imageid = 0;
}


grund_t::~grund_t()
{
	destroy_win((ptrdiff_t)this);

	// remove text from table
	set_text(NULL);

	if(flags&is_halt_flag && !welt->is_destroying()) {
		get_halt()->rem_grund(this);
	}
}


void grund_t::sort_trees()
{
	if (get_typ() != boden) {
		return;
	}
	uint8 trees = 0, offset = 0;
	for(  int i=0;  i<objlist.get_top();  i++  ) {
		if (obj_bei(i)->get_typ() == obj_t::baum) {
			trees++;
			offset = i;
		}
	}
	if(trees > 1) {
		objlist.sort_trees(offset-trees+1u, trees);
	}
}


void grund_t::rotate90()
{
	pos.rotate90( welt->get_size().y-1 );
	slope = slope_t::rotate90( slope );
	// then rotate the things on this tile
	uint8 trees = 0, offset = 0;
	if(  get_top()==254  ) {
		dbg->warning( "grund_t::rotate90()", "Too many stuff on (%s)", pos.get_str() );
	}
	for(  uint8 i=0;  i<objlist.get_top();  i++  ) {
		obj_bei(i)->rotate90();
		if (obj_bei(i)->get_typ() == obj_t::baum) {
			trees++;
			offset = i;
		}
	}
	// if more than one tree on a tile .. resort since offsets changed
	if(trees > 1) {
		objlist.sort_trees(offset-trees+1u, trees);
	}
}


// after processing the last tile, we recalculate the hashes of the ground texts
void grund_t::finish_rotate90()
{
	typedef inthashtable_tpl<uint64, char*> text_map;
	text_map ground_texts_rotating;
	// first get the old hashes
	FOR(text_map, iter, ground_texts) {
		koord3d k = get_ground_koord3d_key( iter.key );
		k.rotate90( welt->get_size().y-1 );
		ground_texts_rotating.put( get_ground_text_key(k), iter.value );
	}
	ground_texts.clear();
	// then transfer all rotated texts
	FOR(text_map, const& iter, ground_texts_rotating) {
		ground_texts.put(iter.key, iter.value);
	}
	ground_texts_rotating.clear();
}


void grund_t::enlarge_map( sint16, sint16 /*new_size_y*/ )
{
	typedef inthashtable_tpl<uint64, char*> text_map;
	text_map ground_texts_enlarged;
	// we have recalculate the keys
	FOR(text_map, iter, ground_texts) {
		koord3d k = get_ground_koord3d_key( iter.key );
		ground_texts_enlarged.put( get_ground_text_key(k), iter.value );
	}
	ground_texts.clear();
	// then transfer all texts back
	FOR(text_map, const& iter, ground_texts_enlarged) {
		ground_texts.put(iter.key, iter.value);
	}
	ground_texts_enlarged.clear();
}


// moves all objects from the old to the new grund_t
void grund_t::take_obj_from(grund_t* other_gr)
{
	// transfer all things
	while( other_gr->get_top() ) {
		objlist.add( other_gr->obj_remove_top() );
	}
	// transfer the way flags
	if(other_gr->get_flag(has_way1)) {
		flags |= has_way1;
		other_gr->clear_flag(has_way1);
	}
	if(other_gr->get_flag(has_way2)) {
		flags |= has_way2;
		other_gr->clear_flag(has_way2);
	}
}


void grund_t::show_info()
{
	int old_count = win_get_open_count();
	if(get_halt().is_bound()) {
		get_halt()->show_info();
		if(env_t::single_info  &&  old_count!=win_get_open_count()  ) {
			return;
		}
	}
	if(env_t::ground_info  ||  hat_wege()) {
		create_win(new grund_info_t(this), w_info, (ptrdiff_t)this);
	}
}


void grund_t::info(cbuffer_t& buf, bool dummy) const
{
	stadt_t* city = welt->get_city(get_pos().get_2d());
	if(city)
	{
		buf.append(city->get_name());
	}
	else
	{
		if(is_water())
		{
			buf.append(translator::translate("Open water"));
		}
		else
		{
			buf.append(translator::translate("Open countryside"));
		}
	}
	buf.append("\n\n");
	bool has_way = false;
	if(!is_water()) {
		if(flags&has_way1) {
			buf.append(translator::translate(get_weg_nr(0)->get_name()));
			buf.append("\n");
			obj_bei(0)->info(buf, ist_bruecke());
			has_way = true;
			if(flags&has_way2) {
				buf.append(translator::translate(get_weg_nr(1)->get_name()));
				buf.append("\n");
				obj_bei(1)->info(buf, ist_bruecke());
				buf.append("\n");
				if(ist_uebergang()) {
					crossing_t* crossing = find<crossing_t>(2);
					buf.append(translator::translate(crossing->get_name()));
					buf.append("\n");
					crossing->get_logic()->info(buf);
					buf.append("\n");
				}
			}
		}
	}

	buf.printf("%s\n%s", translator::translate(get_name()), translator::translate(ground_desc_t::get_climate_name_from_bit(welt->get_climate(get_pos().get_2d()))) );
	buf.append("\n\n");
	if (!is_water())
	{
		char price[64];
		money_to_string(price, abs(welt->get_land_value(pos)));
		buf.printf("%s: %s\n", translator::translate("Land value"), price);
		if (!has_way || (flags&has_way1 && get_weg_nr(0)->is_degraded()) || (flags&has_way2 && get_weg_nr(1)->is_degraded()))
		{
			buf.printf("\n%s:\n", translator::translate("forge_costs"));
			for (int i = 0; i < waytype_t::any_wt; i++)
			{
				char waytype_name[32] = "\0";
				switch (waytype_t(i))
				{
				case tram_wt:	sprintf(waytype_name, "cap_tram_track"); break;
				case track_wt:	sprintf(waytype_name, "cap_track"); break;
				case monorail_wt: sprintf(waytype_name, "cap_monorail_track"); break;
				case maglev_wt: sprintf(waytype_name, "cap_maglev_track"); break;
				case narrowgauge_wt: sprintf(waytype_name, "cap_narrowgauge_track"); break;
				case road_wt:	sprintf(waytype_name, "cap_road"); break;
				case water_wt:	sprintf(waytype_name, "cap_water_canal"); break;
				case air_wt:	sprintf(waytype_name, "cap_taxiway/runway"); break;
				}

				if (waytype_name[1] != '\0')
				{
					if (welt->get_forge_cost(waytype_t(i), pos) < 1000)
					{
						buf.append("  "); // To make the simucent signs position below each other
					}
					money_to_string(price, welt->get_forge_cost(waytype_t(i), pos));
					buf.printf("  %s - %s\n",price, translator::translate(waytype_name));
					if (welt->get_forge_cost(waytype_t(i), pos) == 0)
					{
						buf.printf("       %s\n", translator::translate("already_forged"));
					}
					else if (welt->is_forge_cost_reduced(waytype_t(i), pos))
					{
						buf.printf("       %s\n", translator::translate("partly_forged"));
					}
				}
			}
		}
	}
#if MSG_LEVEL >= 4
	buf.printf("\nflags $%0X", flags );
	buf.printf("\n\npos: (%s)",pos.get_str());
	buf.printf("\nslope: %i",get_grund_hang());
	buf.printf("\nback0: %i",abs(back_imageid)%11);
	buf.printf("\nback1: %i",(abs(back_imageid)/11)+11);
	if(  get_weg_nr(0)  ) {
		buf.printf("\nway slope %i", (int)get_weg_hang() );
	}
	if(get_weg_ribi_unmasked(water_wt)) {
		buf.printf("\nwater ribi: %i",get_weg_ribi_unmasked(water_wt));
	}
	if (is_water()) {
		buf.printf("\ncanal ribi: %i", ((const wasser_t*)this)->get_canal_ribi());
	}
	buf.printf("\ndraw_as_obj= %i",(flags&draw_as_obj)!=0);
	buf.append("\nclimates= ");
	bool following = false;

	climate cl = welt->get_climate( get_pos().get_2d() );
	for(  uint16 i=0;  i < MAX_CLIMATES;  i++  ) {
		if(  cl & (1<<i)  ) {
			if(  following  ) {
				buf.append( ", " );
			}
			following = true;
			buf.append( translator::translate( ground_desc_t::get_climate_name_from_bit((climate)i) ) );
		}
	}
#endif
}


void grund_t::set_halt(halthandle_t halt)
{
	bool add = halt.is_bound();
	if(  add  ) {
		// ok, we want to add a stop: first check if it can apply to water
		if(  get_weg_ribi(water_wt)  ||  is_water()  ||  (ist_karten_boden()  &&  welt->get_climate(pos.get_2d())==water_climate)  ) {
			add = (halt->get_station_type() & haltestelle_t::dock) > 0;
		}
	}
	// then add or remove halt flag
	// and record the halt
	if(  add  ) {
		this_halt = halt;
		flags |= is_halt_flag|dirty;
	}
	else {
		this_halt = halthandle_t();
		flags &= ~is_halt_flag;
		flags |= dirty;
	}
}



halthandle_t grund_t::get_halt() const
{
	return (flags&is_halt_flag) ? this_halt : halthandle_t();
}


// ----------------------- image calculation stuff from here ------------------


void grund_t::calc_image()
{
	// will automatically recalculate ways ...
	objlist.calc_image();
	// since bridges may alter images of ways, this order is needed!
	calc_image_internal( false );
}


void grund_t::set_underground_mode(const uint8 ugm, const sint8 level)
{
	underground_mode = ugm;
	switch(ugm) {
		case ugm_all:
			underground_level = -128;
			break;
		case ugm_level:
			underground_level = level;
			break;
		case ugm_none:
		default:
			underground_mode = ugm_none;
			underground_level = 127;
	}
}


image_id grund_t::get_back_image(int leftback) const
{
	if(back_imageid==0) {
		return IMG_EMPTY;
	}
	sint8 back_image = abs(back_imageid);
	back_image = leftback ? (back_image/11)+11 : back_image%11;
	if(back_imageid<0) {
		return ground_desc_t::fundament->get_image(back_image);
	}
	else {
		return ground_desc_t::slopes->get_image(back_image);
	}
}


// with double height ground tiles!
// can also happen with single height tiles
static inline uint8 get_back_image_from_diff(sint8 h1, sint8 h2)
{
	sint8 min_diff = min( h1, h2 );
	while(  min_diff > 2  ||  (min_diff > 0  &&  h1 != h2)  ) {
		h1 -= min_diff > 1 ? 2 : 1;
		h2 -= min_diff > 1 ? 2 : 1;
		min_diff -= 2;
	}

	if(h1*h2<0) {
		// middle slop of double height
		return h1<0 ? 9 : 10;
	}
	else {
		return (h1>0?h1:0)+(h2>0?h2:0)*3;
	}
}

/**
* if ground is deleted mark the old spot as dirty
*/
void grund_t::mark_image_dirty()
{
	// see obj_t::mark_image_dirty
	if(imageid!=IMG_EMPTY) {
		const scr_coord scr_pos = welt->get_viewport()->get_screen_coord(koord3d(pos.get_2d(),get_disp_height()));
		display_mark_img_dirty( imageid, scr_pos.x, scr_pos.y );
	}
}

#if COLOUR_DEPTH == 0
void grund_t::calc_back_image(const sint8 hgt, const slope_t::type slope_this)
{
	back_imageid = IMG_EMPTY;
}
#else
// artificial walls from here on ...
void grund_t::calc_back_image(const sint8 hgt, const slope_t::type slope_this)
{
	// full underground mode or not ground -> no back image, no need for draw_as_obj
	if (underground_mode == ugm_all || !ist_karten_boden()) {
		clear_flag(grund_t::draw_as_obj);
		this->back_imageid = 0;
		return;
	}

	// store corner heights sw, nw, ne scaled to screen dimensions
	const sint16 scale_z_step = tile_raster_scale_y(TILE_HEIGHT_STEP, 64);
	const sint16 scale_y_step = 64 / 2;
	sint16 corners[3] = { (sint16)(scale_z_step*(hgt + corner_sw(slope_this))),
		(sint16)(scale_z_step*(hgt + corner_nw(slope_this))),
		(sint16)(scale_z_step*(hgt + corner_ne(slope_this))) };
	sint16 corners_add[3] = { 0,0,0 }; // extra height of possible back-image

									   // now calculate back image
	sint8 back_imageid = 0;
	bool is_building = get_typ() == grund_t::fundament;
	const bool isvisible = is_visible();
	bool fence[2] = {false, false};
	const koord k = get_pos().get_2d();

	clear_flag(grund_t::draw_as_obj);
	weg_t const* w;
	if(  (  (w = get_weg_nr(0))  &&  w->get_desc()->is_draw_as_obj()  )  ||
			(  (w = get_weg_nr(1))  &&  w->get_desc()->is_draw_as_obj()  )
	  ) {
		set_flag(grund_t::draw_as_obj);
	}

	for (int i = 0; i<2; i++) {
		// now enter the left/back two height differences
		if (const grund_t *gr = welt->lookup_kartenboden(k + koord::nsew[(i - 1) & 3])) {
			const uint8 back_height = min(corner_nw(slope_this), (i == 0 ? corner_sw(slope_this) : corner_ne(slope_this)));

			const sint16 left_hgt = gr->get_disp_height() - back_height;
			const sint8 slope = gr->get_disp_slope();

			const uint8 corner_a = (i == 0 ? corner_sw(slope_this) : corner_nw(slope_this)) - back_height;
			const uint8 corner_b = (i == 0 ? corner_nw(slope_this) : corner_ne(slope_this)) - back_height;

			sint8 diff_from_ground_1 = left_hgt + (i == 0 ? corner_se(slope) : corner_sw(slope)) - hgt;
			sint8 diff_from_ground_2 = left_hgt + (i == 0 ? corner_ne(slope) : corner_se(slope)) - hgt;

			if (underground_mode == ugm_level) {
				// if exactly one of (this) and (gr) is visible, show full walls
				if (isvisible && !gr->is_visible()) {
					diff_from_ground_1 += 1;
					diff_from_ground_2 += 1;
					set_flag(grund_t::draw_as_obj);
					fence[i] = corner_a == corner_b;
				}
				else if (!isvisible && gr->is_visible()) {
					diff_from_ground_1 = max(diff_from_ground_1, 1);
					diff_from_ground_2 = max(diff_from_ground_2, 1);
				}
				// avoid walls that cover the tunnel mounds
				if (gr->is_visible() && (gr->get_typ() == grund_t::tunnelboden) && ist_karten_boden() && gr->get_pos().z == underground_level
					&& corner_se(gr->get_grund_hang()) > 0 /* se corner */) {
					diff_from_ground_1 = 0;
					diff_from_ground_2 = 0;
				}
				if (is_visible() && (get_typ() == grund_t::tunnelboden) && ist_karten_boden() && pos.z == underground_level
					&& corner_nw(get_grund_hang()) > 0 /* nw corner */) {

					if ((i == 0) ^ (corner_sw(get_grund_hang()) == 0)) {
						diff_from_ground_1 = 0;
						diff_from_ground_2 = 0;
					}
				}
			}

			// up slope hiding something ...
			if (diff_from_ground_1 - corner_a<0 || diff_from_ground_2 - corner_b<0) {
				if (corner_a == corner_b) {
					// ok, we need a fence here, if there is not a vertical bridgehead
					weg_t const* w;
					fence[i] = !(w = get_weg_nr(0)) || (
						!(w->get_ribi_unmasked() & ribi_t::nsew[(i - 1) & 3]) &&
						(!(w = get_weg_nr(1)) || !(w->get_ribi_unmasked() & ribi_t::nsew[(i - 1) & 3]))
						);

					// no fences between water tiles or between invisible tiles
					if (fence[i] && ((is_water() && gr->is_water()) || (!isvisible && !gr->is_visible()))) {
						fence[i] = false;
					}
				}
			}
			// any height difference AND something to see?
			if ((diff_from_ground_1 - corner_a>0 || diff_from_ground_2 - corner_b>0)
				&& (diff_from_ground_1>0 || diff_from_ground_2>0)) {
				back_imageid += get_back_image_from_diff(diff_from_ground_1, diff_from_ground_2)*(i == 0 ? 1 : 11);
				is_building |= gr->get_typ() == grund_t::fundament;
			}
			// update corner heights
			if (diff_from_ground_1 > corner_a) {
				corners_add[i] = max(corners_add[i], scale_z_step * (diff_from_ground_1 - corner_a));
			}
			if (diff_from_ground_2 > corner_b) {
				corners_add[i + 1] = max(corners_add[i + 1], scale_z_step * (diff_from_ground_2 - corner_b));
			}
		}
	}

	for (int i = 0; i<3; i++) {
		corners[i] += corners_add[i];
	}
	// now test more tiles behind whether they are hidden by this tile
	const koord  testdir[3] = { koord(-1,0), koord(-1,-1), koord(0,-1) };

	for (int step = 0; step<5 && !get_flag(draw_as_obj); step++) {
		sint16 test[3] = { (sint16)(corners[0] + 1), (sint16)(corners[1] + 1), (sint16)(corners[2] + 1) };
		for (int i = 0; i <= 2; i++) {
			if (const grund_t *gr = welt->lookup_kartenboden(k + testdir[i] - koord(step, step))) {
				sint16 h = gr->get_disp_height()*scale_z_step;
				sint8 s = gr->get_disp_slope();
				// take backimage into account, take base-height of back image as corner heights
				sint8 bb = abs(gr->back_imageid);
				sint8 lh = 2, rh = 2; // height of start of back image
				if (bb  &&  bb <= 121) {
					if (bb % 11) {
						lh = min(corner_sw(s), corner_nw(s));
					}
					if (bb / 11) {
						rh = min(corner_ne(s), corner_nw(s));
					}
				}
				if (i>0) {
					test[i - 1] = min(test[i - 1], h + min(corner_sw(s), lh)*scale_z_step + (2 - i)*scale_y_step + step*scale_y_step);
				}
				test[i] = min(test[i], h + min(corner_se(s)*scale_z_step, min(min(corner_nw(s), lh), rh)*scale_z_step + scale_y_step) + step*scale_y_step);
				if (i<2) {
					test[i + 1] = min(test[i + 1], h + min(corner_ne(s), rh)*scale_z_step + i*scale_y_step + step*scale_y_step);
				}
			}
		}
		if (test[0] < corners[0] || test[1] < corners[1] || test[2] < corners[2]) {
			// we hide something behind
			set_flag(draw_as_obj);
		}
		else if (test[0] > corners[0] && test[1] > corners[1] && test[2] > corners[2]) {
			// we cannot hide anything anymore
			break;
		}
	}

	// needs a fence?
	if (back_imageid == 0) {
		sint8 fence_offset = fence[0] + 2 * fence[1];
		if (fence_offset) {
			back_imageid = 121 + fence_offset;
		}
	}
	this->back_imageid = (is_building != 0) ? -back_imageid : back_imageid;
}
#endif

#ifdef MULTI_THREAD
void grund_t::display_boden(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, const sint8 clip_num, const bool force_show_grid ) const
#else
void grund_t::display_boden(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width) const
#endif
{
	const bool dirty = get_flag(grund_t::dirty);
	const koord k = get_pos().get_2d();

	// here: we are either ground(kartenboden) or visible
	const bool visible = !ist_karten_boden()  ||  is_karten_boden_visible();

	// walls, fences, foundations etc
	if(back_imageid!=0) {
		const uint8 abs_back_imageid = abs(back_imageid);
		const bool artificial = back_imageid < 0;
		if(abs_back_imageid>121) {
			// fence before a drop
			const sint16 offset = -tile_raster_scale_y( TILE_HEIGHT_STEP*corner_nw(get_grund_hang()), raster_tile_width);
			display_normal( ground_desc_t::fences->get_image( abs_back_imageid + (artificial ? -122 + 3 : -122) ), xpos, ypos + offset, 0, true, dirty CLIP_NUM_PAR );
		}
		else {
			// artificial slope
			const int back_image[2] = {abs_back_imageid%11, (abs_back_imageid/11)+11};

			// choose foundation or natural slopes
			const ground_desc_t *sl_draw = artificial ? ground_desc_t::fundament : ground_desc_t::slopes;

			const sint8 disp_slope = get_disp_slope();
			// first draw left, then back slopes
			for(  int i=0;  i<2;  i++  ) {
				const uint8 back_height = min(i==0?corner_sw(disp_slope):corner_ne(disp_slope),corner_nw(disp_slope));

				if (back_height + get_disp_height() > underground_level) {
					continue;
				}

				sint16 yoff = tile_raster_scale_y( -TILE_HEIGHT_STEP*back_height, raster_tile_width );
				if(  back_image[i]  ) {
					grund_t *gr = welt->lookup_kartenboden( k + koord::nsew[(i-1)&3] );
					if(  gr  ) {
						// for left we test corners 2 and 3 (east), for back we use 1 and 2 (south)
						const sint8 gr_slope = gr->get_disp_slope();
						uint8 corner_a = corner_se(gr_slope);
						uint8 corner_b = i==0?corner_ne(gr_slope):corner_sw(gr_slope);

						// at least one level of solid wall between invisible and visible tiles
						if (!visible  &&  gr->is_visible()) {
							corner_a = max(1, corner_a);
							corner_b = max(1, corner_b);
						}

						sint16 hgt_diff = gr->get_disp_height() - get_disp_height() + min( corner_a, corner_b ) - back_height;
						while(  hgt_diff > 2  ||  (hgt_diff > 0  &&  corner_a != corner_b)  ) {
							uint16 img_index = 22+(hgt_diff>1)+2*i;
							if( sl_draw->get_image( img_index ) == IMG_EMPTY ) {
								img_index = 4+4*(hgt_diff>1)+11*i;
							}
							display_normal( sl_draw->get_image( img_index ), xpos, ypos + yoff, 0, true, dirty CLIP_NUM_PAR );
							yoff     -= tile_raster_scale_y( TILE_HEIGHT_STEP * (hgt_diff > 1 ? 2 : 1), raster_tile_width );
							hgt_diff -= 2;
						}
					}
					display_normal( sl_draw->get_image( back_image[i] ), xpos, ypos + yoff, 0, true, dirty CLIP_NUM_PAR );
				}
			}
		}
	}

	// ground
	image_id image = get_image();
	if(image==IMG_EMPTY) {
		// only check for forced redraw (of marked ... )
		if(dirty) {
			mark_rect_dirty_clip( xpos, ypos + raster_tile_width / 2, xpos + raster_tile_width - 1, ypos + raster_tile_width - 1 CLIP_NUM_PAR );
		}
	}
	else {
		if(get_typ()!=wasser) {
			// show image if tile is visible
			if (visible)  {
				display_normal( get_image(), xpos, ypos, 0, true, dirty CLIP_NUM_PAR );
#ifdef SHOW_FORE_GRUND
				if (get_flag(grund_t::draw_as_obj)) {
					display_blend( get_image(), xpos, ypos, 0, COL_RED | OUTLINE_FLAG |TRANSPARENT50_FLAG, true, dirty CLIP_NUM_PAR );
				}
#endif
				//display climate transitions - only needed if below snowline (snow_transition>0)
				//need to process whole tile for all heights anyway as water transitions are needed for all heights
				const planquadrat_t * plan = welt->access( k );
				uint8 climate_corners = plan->get_climate_corners();
				const sint8 snow_transition = welt->get_snowline() - pos.z;
				weg_t *weg = get_weg(road_wt);
				if(  climate_corners != 0  &&  (!weg  ||  !weg->hat_gehweg())  ) {
					uint8 water_corners = 0;

					// get neighbour corner heights
					sint8 neighbour_height[8][4];
					welt->get_neighbour_heights( k, neighbour_height );

					//look up neighbouring climates
					climate neighbour_climate[8];
					for(  int i = 0;  i < 8;  i++  ) {
						koord k_neighbour = k + koord::neighbours[i];
						if(  !welt->is_within_limits(k_neighbour)  ) {
							k_neighbour = welt->get_closest_coordinate(k_neighbour);
						}
						neighbour_climate[i] = welt->get_climate( k_neighbour );
					}

					climate climate0 = plan->get_climate();
					slope_t::type slope_corner = get_grund_hang();

					// get transition climate - look for each corner in turn
					for(  int i = 0;  i < 4;  i++  ) {
						sint8 corner_height = get_hoehe() + (slope_corner % 3);

						climate transition_climate = climate0;
						climate min_climate = arctic_climate;

						for(  int j = 1;  j < 4;  j++ ) {
							if(  corner_height == neighbour_height[(i * 2 + j) & 7][(i + j) & 3]) {
								climate climatej = neighbour_climate[(i * 2 + j) & 7];
								climatej > transition_climate ? transition_climate = climatej : 0;
								climatej < min_climate ? min_climate = climatej : 0;
							}
						}

						if(  min_climate == water_climate  ) {
							water_corners += 1 << i;
						}
						if(  (climate_corners >> i) & 1  &&  !is_water()  &&  snow_transition > 0  ) {
							// looks up sw, se, ne, nw for i=0...3
							// we compare with tile either side (e.g. for sw, w and s) and pick highest one
							if(  transition_climate > climate0  ) {
								uint8 overlay_corners = 1 << i;
								slope_t::type slope_corner_se = slope_corner;
								for(  int j = i + 1;  j < 4;  j++  ) {
									slope_corner_se /= 3;

									// now we check to see if any of remaining corners have same climate transition (also using highest of course)
									// if so we combine into this overlay layer
									if(  (climate_corners >> j) & 1  ) {
										climate compare = climate0;
										for(  int k = 1;  k < 4;  k++  ) {
											corner_height = get_hoehe() + (slope_corner_se % 3);
											if(  corner_height == neighbour_height[(j * 2 + k) & 7][(j + k) & 3]) {
												climate climatej = neighbour_climate[(j * 2 + k) & 7];
												climatej > compare ? compare = climatej : 0;
											}
										}

										if(  transition_climate == compare  ) {
											overlay_corners += 1 << j;
											climate_corners -= 1 << j;
										}
									}
								}
								// overlay transition climates
								display_alpha( ground_desc_t::get_climate_tile( transition_climate, slope ), ground_desc_t::get_alpha_tile( slope, overlay_corners ), ALPHA_GREEN | ALPHA_BLUE, xpos, ypos, 0, 0, true, dirty CLIP_NUM_PAR );
							}
						}
						slope_corner /= 3;
					}
					// finally overlay any water transition
					if(  water_corners  ) {
						if(  slope  ) {
							display_alpha( ground_desc_t::get_water_tile(slope), ground_desc_t::get_beach_tile( slope, water_corners ), ALPHA_RED, xpos, ypos, 0, 0, true, dirty CLIP_NUM_PAR );
							if(  ground_desc_t::shore  ) {
								// additional shore image
								image_id shore = ground_desc_t::shore->get_image(slope,snow_transition<=0);
								if(  shore==IMG_EMPTY  &&  snow_transition<=0  ) {
									shore = ground_desc_t::shore->get_image(slope,0);
								}
								display_normal( shore, xpos, ypos, 0, true, dirty CLIP_NUM_PAR );
							}
						}
						else {
							// animate
							display_alpha( ground_desc_t::sea->get_image(0,wasser_t::stage), ground_desc_t::get_beach_tile( slope, water_corners ), ALPHA_RED, xpos, ypos, 0, 0, true, dirty|wasser_t::change_stage CLIP_NUM_PAR );
						}
					}
				}

				//display snow transitions if required
				if(  slope != 0  &&  (!weg  ||  !weg->hat_gehweg())  ) {
					switch(  snow_transition  ) {
						case 1: {
							display_alpha( ground_desc_t::get_snow_tile(slope), ground_desc_t::get_alpha_tile(slope), ALPHA_GREEN | ALPHA_BLUE, xpos, ypos, 0, 0, true, dirty CLIP_NUM_PAR );
							break;
						}
						case 2: {
							if(  slope_t::max_diff(slope) > 1  ) {
								display_alpha( ground_desc_t::get_snow_tile(slope), ground_desc_t::get_alpha_tile(slope), ALPHA_BLUE, xpos, ypos, 0, 0, true, dirty CLIP_NUM_PAR );
							}
							break;
						}
					}
				}

				// we show additionally a grid
				// for undergroundmode = ugm_all the grid is plotted in display_obj
#ifdef MULTI_THREAD
				if(  show_grid  || force_show_grid  ) {
#else
				if(  show_grid  ){
#endif
					const uint8 hang = get_grund_hang();
					display_normal( ground_desc_t::get_border_image(hang), xpos, ypos, 0, true, dirty CLIP_NUM_PAR );
				}
			}
		}
		else {
			// take animation into account
			if(  underground_mode != ugm_all  ) {
				display_normal( ground_desc_t::sea->get_image(get_image(),wasser_t::stage), xpos, ypos, 0, true, dirty|wasser_t::change_stage CLIP_NUM_PAR );
			}
			else {
				display_blend( ground_desc_t::sea->get_image(get_image(),wasser_t::stage), xpos, ypos, 0, TRANSPARENT50_FLAG, true, dirty|wasser_t::change_stage CLIP_NUM_PAR );
			}
			return;
		}
	}
	// display ways
	if(  visible  &&  (flags&has_way1)  ){
		const bool clip = (  (flags&draw_as_obj)  ||  !ist_karten_boden()  )  &&  !env_t::simple_drawing;
		const int hgt_step = tile_raster_scale_y( TILE_HEIGHT_STEP, raster_tile_width );
		for(  uint8 i = 0;  i < offsets[flags / has_way1];  i++  ) {
			obj_t* d = obj_bei(i);
			// clip
			// .. nonconvex n/w if not both n/w are active
			if(  clip  ) {
				const ribi_t::ribi way_ribi = (static_cast<const weg_t*>(d))->get_ribi_unmasked();
				clear_all_poly_clip( CLIP_NUM_VAR );
				const uint8 non_convex = (way_ribi & ribi_t::northwest) == ribi_t::northwest ? 0 : 16;
				if(  way_ribi & ribi_t::west  ) {
					const int dh = corner_nw(get_disp_way_slope()) * hgt_step;
					add_poly_clip( xpos + raster_tile_width / 2 - 1, ypos + raster_tile_width / 2 - dh, xpos - 1, ypos + 3 * raster_tile_width / 4 - dh, ribi_t::west | non_convex CLIP_NUM_PAR );
				}
				if(  way_ribi & ribi_t::north  ) {
					const int dh = corner_nw(get_disp_way_slope()) * hgt_step;
					add_poly_clip(xpos + raster_tile_width - 1, ypos + 3 * raster_tile_width / 4 - 1 - dh, xpos + raster_tile_width / 2 + 1, ypos + raster_tile_width / 2 - dh, ribi_t::north | non_convex CLIP_NUM_PAR);
				}
				activate_ribi_clip((way_ribi & ribi_t::northwest) | non_convex  CLIP_NUM_PAR);
			}
			d->display( xpos, ypos CLIP_NUM_PAR );
		}
		// end of clipping
		if(  clip  ) {
			clear_all_poly_clip( CLIP_NUM_VAR );
		}
	}
}



void grund_t::display_border( sint16 xpos, sint16 ypos, const sint16 raster_tile_width CLIP_NUM_DEF)
{
	if(  pos.z < welt->get_groundwater()  ) {
		// we do not display below water (yet)
		return;
	}

	const sint16 hgt_step = tile_raster_scale_y( TILE_HEIGHT_STEP, raster_tile_width);
	static sint8 lookup_hgt[5] = { 6, 3, 0, 1, 2 };

	if(  pos.y-welt->get_size().y+1 == 0  ) {
		// move slopes to front of tile
		sint16 x = xpos - raster_tile_width/2;
		sint16 y = ypos + raster_tile_width/4 + (pos.z-welt->get_groundwater())*hgt_step;
		// left side border
		sint16 diff = corner_sw(slope)-corner_se(slope);
		image_id slope_img = ground_desc_t::slopes->get_image( lookup_hgt[ 2+diff ]+11 );
		diff = -min(corner_sw(slope),corner_se(slope));
		sint16 zz = pos.z-welt->get_groundwater();
		if(  diff < zz && ((zz-diff)&1)==1  ) {
			display_normal( ground_desc_t::slopes->get_image(15), x, y, 0, true, false CLIP_NUM_PAR );
			y -= hgt_step;
			diff++;
		}
		// ok, now we have the height; since the slopes may end with a fence they are drawn in reverse order
		while(  diff < zz  ) {
			display_normal( ground_desc_t::slopes->get_image(19), x, y, 0, true, false CLIP_NUM_PAR );
			y -= hgt_step*2;
			diff+=2;
		}
		display_normal( slope_img, x, y, 0, true, false CLIP_NUM_PAR );
	}

	if(  pos.x-welt->get_size().x+1 == 0  ) {
		// move slopes to front of tile
		sint16 x = xpos + raster_tile_width/2;
		sint16 y = ypos + raster_tile_width/4 + (pos.z-welt->get_groundwater())*hgt_step;
		// right side border
		sint16 diff = corner_se(slope)-corner_ne(slope);
		image_id slope_img = ground_desc_t::slopes->get_image( lookup_hgt[ 2+diff ] );
		diff = -min(corner_se(slope),corner_ne(slope));
		sint16 zz = pos.z-welt->get_groundwater();
		if(  diff < zz && ((zz-diff)&1)==1  ) {
			display_normal( ground_desc_t::slopes->get_image(4), x, y, 0, true, false CLIP_NUM_PAR );
			y -= hgt_step;
			diff++;
		}
		// ok, now we have the height; since the slopes may end with a fence they are drawn in reverse order
		while(  diff < zz  ) {
			display_normal( ground_desc_t::slopes->get_image(8), x, y, 0, true, false CLIP_NUM_PAR );
			y -= hgt_step*2;
			diff+=2;
		}
		display_normal( slope_img, x, y, 0, true, false CLIP_NUM_PAR );
	}
}


#ifdef MULTI_THREAD
void grund_t::display_if_visible(sint16 xpos, sint16 ypos, const sint16 raster_tile_width, const sint8 clip_num, const bool force_show_grid )
#else
void grund_t::display_if_visible(sint16 xpos, sint16 ypos, const sint16 raster_tile_width)
#endif
{
	if(  !is_karten_boden_visible()  ) {
		// only check for forced redraw (of marked ... )
		if(dirty) {
			mark_rect_dirty_clip( xpos, ypos + raster_tile_width / 2, xpos + raster_tile_width - 1, ypos + raster_tile_width - 1 CLIP_NUM_PAR );
		}
		return;
	}

	if(  env_t::draw_earth_border  &&  (pos.x-welt->get_size().x+1 == 0  ||  pos.y-welt->get_size().y+1 == 0)  ) {
		// the last tile. might need a border
		display_border( xpos, ypos, raster_tile_width CLIP_NUM_PAR );
	}

	if(!get_flag(grund_t::draw_as_obj)) {
#ifdef MULTI_THREAD
		display_boden( xpos, ypos, raster_tile_width, clip_num, force_show_grid );
#else
		display_boden( xpos, ypos, raster_tile_width );
#endif
	}
}


slope_t::type grund_t::get_disp_way_slope() const
{
	if (is_visible()) {
		if (ist_bruecke()) {
			const slope_t::type slope = get_grund_hang();
			if(  slope != 0  ) {
				// for half height slopes we want all corners at 1, for full height all corners at 2
				return (slope & 7) ? slope_t::raised / 2 : slope_t::raised;
			}
			else {
				return get_weg_hang();
			}
		}
		else if (ist_tunnel() && ist_karten_boden()) {
			if (pos.z >= underground_level) {
				return slope_t::flat;
			}
			else {
				return get_grund_hang();
			}
		}
		else {
			return get_grund_hang();
		}
	}
	else {
		return slope_t::flat;
	}
}



/**
 * The old main display routine. Used for very small tile sizes, where clipping error
 * will be only one or two pixels.
 *
 * Also used in multi-threaded display.
 */
void grund_t::display_obj_all_quick_and_dirty(const sint16 xpos, sint16 ypos, const sint16 raster_tile_width, const bool is_global CLIP_NUM_DEF ) const
{
	const bool dirty = get_flag(grund_t::dirty);
	const uint8 start_offset=offsets[flags/has_way1];

	// here: we are either ground(kartenboden) or visible
	const bool visible = !ist_karten_boden()  ||  is_karten_boden_visible();

	if(  visible  ) {
		if(  is_global  &&  get_flag( grund_t::marked )  ) {
			const uint8 hang = get_grund_hang();
			display_img_aux( ground_desc_t::get_marker_image( hang, true ), xpos, ypos, 0, true, dirty CLIP_NUM_PAR );
#ifdef MULTI_THREAD
			objlist.display_obj_quick_and_dirty( xpos, ypos, start_offset CLIP_NUM_PAR );
#else
			objlist.display_obj_quick_and_dirty( xpos, ypos, start_offset, is_global );
#endif
			display_img_aux( ground_desc_t::get_marker_image( hang, false ), xpos, ypos, 0, true, dirty CLIP_NUM_PAR );
			if(  !ist_karten_boden()  ) {
				const grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
				if(  pos.z > gr->get_hoehe()  ) {
					//display front part of marker for grunds in between
					for(  sint8 z = pos.z - 1;  z > gr->get_hoehe();  z--  ) {
						display_img_aux( ground_desc_t::get_marker_image(0, false), xpos, ypos - tile_raster_scale_y( (z - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true CLIP_NUM_PAR );
					}
					//display front part of marker for ground
					display_img_aux( ground_desc_t::get_marker_image( gr->get_grund_hang(), false ), xpos, ypos - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true CLIP_NUM_PAR );
				}
				else if(  pos.z < gr->get_disp_height()  ) {
					//display back part of marker for grunds in between
					for(  sint8 z = pos.z + 1;  z < gr->get_disp_height();  z++  ) {
						display_img_aux( ground_desc_t::get_border_image(0), xpos, ypos - tile_raster_scale_y( (z - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true CLIP_NUM_PAR );
					}
					//display back part of marker for ground
					const uint8 kbhang = gr->get_grund_hang() | gr->get_weg_hang();
					display_img_aux( ground_desc_t::get_border_image(kbhang), xpos, ypos - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true CLIP_NUM_PAR );
				}
			}
		}
		else {
#ifdef MULTI_THREAD
			objlist.display_obj_quick_and_dirty( xpos, ypos, start_offset CLIP_NUM_PAR );
#else
			objlist.display_obj_quick_and_dirty( xpos, ypos, start_offset, is_global );
#endif
		}
	}
	else { // must be karten_boden
		// in undergroundmode: draw ground grid
		const uint8 hang = underground_mode==ugm_all ? get_grund_hang() : (uint8)slope_t::flat;
		display_img_aux( ground_desc_t::get_border_image(hang), xpos, ypos, 0, true, dirty CLIP_NUM_PAR );
		// show marker for marked but invisible tiles
		if(  is_global  &&  get_flag(grund_t::marked)  ) {
			display_img_aux( ground_desc_t::get_marker_image( hang, true ), xpos, ypos, 0, true, dirty CLIP_NUM_PAR );
			display_img_aux( ground_desc_t::get_marker_image( hang, false ), xpos, ypos, 0, true, dirty CLIP_NUM_PAR );
		}
	}
}


/* The main display routine

Premise:
-- all objects on one tile are sorted such that are no graphical glitches on the tile
-- all glitches happens with objects that are not on the tile but their graphics is (vehicles on tiles before/after stations)
-- ground graphics is already painted when this routine is called

Idea:
-- clip everything along tile borders that are crossed by ways
-- vehicles of neighboring tiles may overlap our graphic, hence we have to call display-routines of neighboring tiles too

Algorithm:
0) detect way ribis on the tile and prepare the clipping (see simgraph: add_poly_clip, display_img_pc)
1) display our background (for example station graphics background)
2) display vehicles of w/nw/n neighbors (these are behind this tile) (if we have ways in this direction; clipped along the n and/or w tile border)
3) display background of s/e neighbors (clipped - only pixels that are on our tile are painted)
4) display vehicles on this tile (clipped)
5) display vehicles of ne/e/se/s/sw neighbors
6) display our foreground (foreground image of station/overheadwire piles etc) no clipping
*/
void grund_t::display_obj_all(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, const bool is_global CLIP_NUM_DEF ) const
{
	if(  env_t::simple_drawing  ) {
		display_obj_all_quick_and_dirty( xpos, ypos, raster_tile_width, is_global CLIP_NUM_PAR );
		return;
	}

	// end of clipping
	clear_all_poly_clip( CLIP_NUM_VAR );

	// here: we are either ground(kartenboden) or visible
	const bool visible = !ist_karten_boden()  ||  is_karten_boden_visible();

	ribi_t::ribi ribi = ribi_t::none;
	if (weg_t const* const w1 = get_weg_nr(0)) {
		ribi |= w1->get_ribi_unmasked();
		if (weg_t const* const w2 = get_weg_nr(1)) {
			ribi |= w2->get_ribi_unmasked();
		}
	}
	else if (is_water()) {
		ribi = (static_cast<const wasser_t*>(this))->get_weg_ribi(water_wt);
	}

	// now ways? - no clipping needed, avoid all the ribi-checks
	if (ribi==ribi_t::none) {
		// display background
		const uint8 offset_vh = display_obj_bg( xpos, ypos, is_global, true, visible CLIP_NUM_PAR );
		if (visible) {
			// display our vehicles
			const uint8 offset_fg = display_obj_vh( xpos, ypos, offset_vh, ribi, true CLIP_NUM_PAR );
			// foreground
			display_obj_fg( xpos, ypos, is_global, offset_fg CLIP_NUM_PAR );
		}
		return;
	}

	// ships might be large and could be clipped by vertical walls on our tile
	const bool ontile_se = back_imageid  &&  is_water();

	// get slope of way as displayed
	const uint8 slope = get_disp_way_slope();
	// clip
	const int hgt_step = tile_raster_scale_y( TILE_HEIGHT_STEP, raster_tile_width);
	// .. nonconvex n/w if not both n/w are active and if we have back image
	//              otherwise our backwall clips into the part of our back image that is drawn by n/w neighbor
	const uint8 non_convex = ((ribi & ribi_t::northwest) == ribi_t::northwest)  &&  back_imageid ? 0 : 16;
	if(  ribi & ribi_t::west  ) {
		const int dh = corner_nw(slope) * hgt_step;
		add_poly_clip( xpos + raster_tile_width / 2 - 1, ypos + raster_tile_width / 2 - dh, xpos - 1, ypos + 3 * raster_tile_width / 4 - dh, ribi_t::west | non_convex CLIP_NUM_PAR );
	}
	if(  ribi & ribi_t::north  ) {
		const int dh = corner_nw(slope) * hgt_step;
		add_poly_clip(xpos + raster_tile_width - 1, ypos + 3 * raster_tile_width / 4 - 1 - dh, xpos + raster_tile_width / 2 + 1, ypos + raster_tile_width / 2 - dh, ribi_t::north | non_convex CLIP_NUM_PAR);
	}
	if(  ribi & ribi_t::east  ) {
		const int dh = corner_se(slope) * hgt_step;
		add_poly_clip(xpos + raster_tile_width / 2, ypos + raster_tile_width - dh, xpos + raster_tile_width, ypos + 3 * raster_tile_width / 4 - dh, ribi_t::east | 16 CLIP_NUM_PAR);
	}
	if(  ribi & ribi_t::south  ) {
		const int dh = corner_se(slope) * hgt_step;
		add_poly_clip(xpos, ypos + 3 * raster_tile_width / 4 + 1 - dh, xpos + raster_tile_width / 2, ypos + raster_tile_width + 1 - dh, ribi_t::south | 16  CLIP_NUM_PAR);
	}
	// display background
	// get offset of first vehicle
	activate_ribi_clip( (ribi_t::northwest & ribi) | 16 CLIP_NUM_PAR );
	const uint8 offset_vh = display_obj_bg( xpos, ypos, is_global, false, visible CLIP_NUM_PAR );
	if(  !visible  ) {
		// end of clipping
		clear_all_poly_clip( CLIP_NUM_VAR );
		return;
	}
	// display vehicles of w/nw/n neighbors
	grund_t *gr_nw = NULL, *gr_ne = NULL, *gr_se = NULL, *gr_sw = NULL;
	if(  ribi & ribi_t::west  ) {
		grund_t *gr;
		if(  get_neighbour( gr, invalid_wt, ribi_t::west )  ) {
			gr->display_obj_vh( xpos - raster_tile_width / 2, ypos - raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::west, false CLIP_NUM_PAR );
			if(  ribi & ribi_t::south  ) {
				gr->get_neighbour( gr_nw, invalid_wt, ribi_t::north );
			}
			if(  ribi & ribi_t::north  ) {
				gr->get_neighbour( gr_sw, invalid_wt, ribi_t::south );
			}
		}
	}
	if(  ribi & ribi_t::north  ) {
		grund_t *gr;
		if(  get_neighbour( gr, invalid_wt, ribi_t::north )  ) {
			gr->display_obj_vh( xpos + raster_tile_width / 2, ypos - raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::north, false CLIP_NUM_PAR );
			if(  (ribi & ribi_t::east)  &&  (gr_nw == NULL)  ) {
				gr->get_neighbour( gr_nw, invalid_wt, ribi_t::west );
			}
			if(  (ribi & ribi_t::west)  ) {
				gr->get_neighbour( gr_ne, invalid_wt, ribi_t::east );
			}
		}
	}
	if(  (ribi & ribi_t::northwest)  &&  gr_nw  ) {
		gr_nw->display_obj_vh( xpos, ypos - raster_tile_width / 2 - tile_raster_scale_y( (gr_nw->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::northwest, false CLIP_NUM_PAR );
	}
	// display background s/e
	if(  ribi & ribi_t::east  ) {
		grund_t *gr;
		if(  get_neighbour( gr, invalid_wt, ribi_t::east )  ) {
			const bool draw_other_ways = (flags&draw_as_obj)  ||  (gr->flags&draw_as_obj)  ||  !gr->ist_karten_boden();
			activate_ribi_clip(ribi_t::east | 16 CLIP_NUM_PAR);
			gr->display_obj_bg( xpos + raster_tile_width / 2, ypos + raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), is_global, draw_other_ways, true CLIP_NUM_PAR );
		}
	}
	if(  ribi & ribi_t::south  ) {
		grund_t *gr;
		if(  get_neighbour( gr, invalid_wt, ribi_t::south )  ) {
			const bool draw_other_ways = (flags&draw_as_obj)  ||  (gr->flags&draw_as_obj)  ||  !gr->ist_karten_boden();
			activate_ribi_clip(ribi_t::south | 16 CLIP_NUM_PAR);
			gr->display_obj_bg( xpos - raster_tile_width / 2, ypos + raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), is_global, draw_other_ways, true CLIP_NUM_PAR );
		}
	}
	// display our vehicles
	const uint8 offset_fg = display_obj_vh( xpos, ypos, offset_vh, ribi, true CLIP_NUM_PAR );

	// display vehicles of ne/e/se/s/sw neighbors
	if(  ribi & ribi_t::east  ) {
		grund_t *gr;
		if(  get_neighbour( gr, invalid_wt, ribi_t::east )  ) {
			gr->display_obj_vh( xpos + raster_tile_width / 2, ypos + raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::east, ontile_se CLIP_NUM_PAR );
			if(  (ribi & ribi_t::south)  &&  (gr_ne == NULL)  ) {
				gr->get_neighbour( gr_ne, invalid_wt, ribi_t::north );
			}
			if(  (ribi & ribi_t::north)  &&  (gr_se == NULL)  ) {
				gr->get_neighbour( gr_se, invalid_wt, ribi_t::south );
			}
		}
	}
	if(  ribi & ribi_t::south  ) {
		grund_t *gr;
		if(  get_neighbour( gr, invalid_wt, ribi_t::south )  ) {
			gr->display_obj_vh( xpos - raster_tile_width / 2, ypos + raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::south, ontile_se CLIP_NUM_PAR );
			if(  (ribi & ribi_t::east)  &&  (gr_sw == NULL)) {
				gr->get_neighbour( gr_sw, invalid_wt, ribi_t::west );
			}
			if(  (ribi & ribi_t::west)  &&  (gr_se == NULL)) {
				gr->get_neighbour( gr_se, invalid_wt, ribi_t::east );
			}
		}
	}
	if(  (ribi & ribi_t::northeast)  &&  gr_ne  ) {
		gr_ne->display_obj_vh( xpos + raster_tile_width, ypos - tile_raster_scale_y( (gr_ne->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::northeast, ontile_se CLIP_NUM_PAR );
	}
	if(  (ribi & ribi_t::southwest)  &&  gr_sw  ) {
		gr_sw->display_obj_vh( xpos - raster_tile_width, ypos - tile_raster_scale_y( (gr_sw->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::southwest, ontile_se CLIP_NUM_PAR );
	}
	if(  (ribi & ribi_t::southeast)  &&  gr_se  ) {
		gr_se->display_obj_vh( xpos, ypos + raster_tile_width / 2 - tile_raster_scale_y( (gr_se->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::southeast, ontile_se CLIP_NUM_PAR );
	}

	// end of clipping
	clear_all_poly_clip( CLIP_NUM_VAR );

	// foreground
	display_obj_fg( xpos, ypos, is_global, offset_fg CLIP_NUM_PAR );
}


uint8 grund_t::display_obj_bg(const sint16 xpos, const sint16 ypos, const bool is_global, const bool draw_ways, const bool visible CLIP_NUM_DEF ) const
{
	const bool dirty = get_flag(grund_t::dirty);

	if(  visible  ) {
		// display back part of markers
		if(  is_global  &&  get_flag( grund_t::marked )  ) {
			display_normal( ground_desc_t::get_marker_image( get_grund_hang(), true ), xpos, ypos, 0, true, dirty CLIP_NUM_PAR );
			if(  !ist_karten_boden()  ) {
				const grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
				const sint16 raster_tile_width = get_current_tile_raster_width();
				if(  pos.z < gr->get_disp_height()  ) {
					//display back part of marker for grunds in between
					for(  sint8 z = pos.z + 1;  z < gr->get_disp_height();  z++  ) {
						display_normal( ground_desc_t::get_marker_image(0, true), xpos, ypos - tile_raster_scale_y( (z - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true CLIP_NUM_PAR );
					}
					//display back part of marker for ground
					display_normal( ground_desc_t::get_marker_image( gr->get_grund_hang() | gr->get_weg_hang(), true ), xpos, ypos - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true CLIP_NUM_PAR );
				}
			}
		}
		// display background images of everything but vehicles
		const uint8 start_offset = draw_ways ? 0 : offsets[flags/has_way1];
		return objlist.display_obj_bg( xpos, ypos, start_offset CLIP_NUM_PAR );
	}
	else { // must be karten_boden
		// in undergroundmode: draw ground grid
		const uint8 hang = underground_mode == ugm_all ? get_grund_hang() : (slope_t::type)slope_t::flat;
		display_normal( ground_desc_t::get_border_image(hang), xpos, ypos, 0, true, dirty CLIP_NUM_PAR );
		// show marker for marked but invisible tiles
		if(  is_global  &&  get_flag( grund_t::marked )  ) {
			display_img_aux( ground_desc_t::get_marker_image( hang, true ), xpos, ypos, 0, true, dirty CLIP_NUM_PAR );
			display_img_aux( ground_desc_t::get_marker_image( hang, false ), xpos, ypos, 0, true, dirty CLIP_NUM_PAR );
		}
		return 255;
	}
}


uint8 grund_t::display_obj_vh(const sint16 xpos, const sint16 ypos, const uint8 start_offset, const ribi_t::ribi ribi, const bool ontile CLIP_NUM_DEF ) const
{
	return objlist.display_obj_vh( xpos, ypos, start_offset, ribi, ontile CLIP_NUM_PAR );
}


void grund_t::display_obj_fg(const sint16 xpos, const sint16 ypos, const bool is_global, const uint8 start_offset CLIP_NUM_DEF ) const
{
	const bool dirty = get_flag(grund_t::dirty);
#ifdef MULTI_THREAD
	objlist.display_obj_fg( xpos, ypos, start_offset CLIP_NUM_PAR );
#else
	objlist.display_obj_fg( xpos, ypos, start_offset, is_global );
#endif
	// display front part of markers
	if(  is_global  &&  get_flag( grund_t::marked )  ) {
		display_normal( ground_desc_t::get_marker_image( get_grund_hang(), false ), xpos, ypos, 0, true, dirty CLIP_NUM_PAR );
		if(  !ist_karten_boden()  ) {
			const grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
			const sint16 raster_tile_width = get_tile_raster_width();
			if(  pos.z > gr->get_hoehe()  ) {
				//display front part of marker for grunds in between
				for(  sint8 z = pos.z - 1;  z > gr->get_hoehe();  z--  ) {
					display_normal( ground_desc_t::get_marker_image( 0, false ), xpos, ypos - tile_raster_scale_y( (z - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true CLIP_NUM_PAR );
				}
				//display front part of marker for ground
				display_normal( ground_desc_t::get_marker_image( gr->get_grund_hang(), false ), xpos, ypos - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true CLIP_NUM_PAR );
			}
		}
	}
}


void grund_t::display_overlay(const sint16 xpos, const sint16 ypos)
{
	const bool dirty = get_flag(grund_t::dirty);
#ifdef MULTI_THREAD
	objlist.display_obj_overlay( xpos, ypos );
#endif
	// marker/station text
	if(  get_flag(has_text)  &&  env_t::show_names  ) {
		if(  env_t::show_names&1  ) {
			const char *text = get_text();
			const sint16 raster_tile_width = get_tile_raster_width();
			const int width = proportional_string_width(text)+7;
			int new_xpos = xpos - (width-raster_tile_width)/2;
			PLAYER_COLOR_VAL pc = text_farbe();

			switch( env_t::show_names >> 2 ) {
				case 0:
					display_ddd_proportional_clip( new_xpos, ypos, width, 0, pc, SYSCOL_TEXT, text, dirty );
					break;
				case 1:
					display_outline_proportional( new_xpos, ypos-(LINESPACE/2), pc+3, SYSCOL_TEXT, text, dirty );
					break;
				case 2:
					display_outline_proportional( 16+new_xpos, ypos-(LINESPACE/2), COL_YELLOW, SYSCOL_TEXT, text, dirty );
					display_ddd_box_clip( new_xpos, ypos-(LINESPACE/2), LINESPACE, LINESPACE, pc-2, pc+2 );
					display_fillbox_wh( new_xpos+1, ypos-(LINESPACE/2)+1, LINESPACE-2, LINESPACE-2, pc, dirty );
					break;
			}
		}

		// display station waiting information/status
		if(env_t::show_names & 2) {
			const halthandle_t halt = get_halt();
			if(halt.is_bound()  &&  halt->get_basis_pos3d()==pos) {
				halt->display_status(xpos, ypos);
			}
		}
	}
	clear_flag(grund_t::dirty);
}


// ---------------- way and wayobj handling -------------------------------


ribi_t::ribi grund_t::get_weg_ribi(waytype_t typ) const
{
	weg_t *weg = get_weg(typ);
	return (weg) ? weg->get_ribi() : (ribi_t::ribi)ribi_t::none;
}


ribi_t::ribi grund_t::get_weg_ribi_unmasked(waytype_t typ) const
{
	weg_t *weg = get_weg(typ);
	return (weg) ? weg->get_ribi_unmasked() : (ribi_t::ribi)ribi_t::none;
}


/**
* If there's a depot here, return this
* @author Volker Meyer
*/
depot_t* grund_t::get_depot() const
{
	return dynamic_cast<depot_t *>(first_obj());
}

signalbox_t* grund_t::get_signalbox() const
{
	return dynamic_cast<signalbox_t *>(first_obj());
}

gebaeude_t *grund_t::get_building() const
{
	gebaeude_t *gb = find<gebaeude_t>();
	if (gb) {
		return gb;
	}

	gb = get_signalbox();
	if(gb)
	{
		return gb;
	}

	return get_depot();
}


bool grund_t::weg_erweitern(waytype_t wegtyp, ribi_t::ribi ribi)
{
	weg_t   *weg = get_weg(wegtyp);
	if(weg) {
		weg->ribi_add(ribi);
		weg->count_sign();
		if(weg->is_electrified()) {
			wayobj_t *wo = get_wayobj( wegtyp );
			if( (ribi & wo->get_dir()) == 0 ) {
				// ribi isn't set at wayobj;
				for( uint8 i = 0; i < 4; i++ ) {
					// Add ribis to adjacent wayobj.
					if( ribi_t::nsew[i] & ribi ) {
						grund_t *next_gr;
						if( get_neighbour( next_gr, wegtyp, ribi_t::nsew[i] ) ) {
							wayobj_t *wo2 = next_gr->get_wayobj( wegtyp );
							if( wo2 ) {
								wo->set_dir( wo->get_dir() | ribi_t::nsew[i] );
								wo2->set_dir( wo2->get_dir() | ribi_t::backward(ribi_t::nsew[i]) );
							}
						}
					}
				}
			}
		}
		calc_image();
		set_flag(dirty);
		return true;
	}
	return false;
}

/**
 * remove trees and groundobjs on this tile
 * called before building way or powerline
 * @return costs
 */
sint64 grund_t::remove_trees()
{
	sint64 cost=0;
	// remove all trees ...
	while (baum_t* const d = find<baum_t>(0)) {
		// we must mark it by hand, since we want to join costs
		d->mark_image_dirty( get_image(), 0 );
		delete d;
		cost -= welt->get_settings().cst_remove_tree;
	}
	// remove all groundobjs ...
	while (groundobj_t* const d = find<groundobj_t>(0)) {
		cost += d->get_desc()->get_value();
		delete d;
	}
	return cost;
}



sint64 grund_t::neuen_weg_bauen(weg_t *weg, ribi_t::ribi ribi, player_t *player, koord3d_vector_t *route)
{
	sint64 cost = 0;

	// not already there?
	const weg_t * alter_weg = get_weg(weg->get_waytype());
	if(alter_weg == NULL)
	{
		// ok, we are unique
		// Calculate the forge cost
		sint64 forge_cost = welt->get_settings().get_forge_cost(weg->get_waytype());

		if(route != NULL)
		{
			// No parallel way discounting for bridges and tunnels.
			for(int n = 0; n < 8; n ++)
			{
				const koord kn = pos.get_2d().neighbours[n] + pos.get_2d();
				if(!welt->is_within_grid_limits(kn))
				{
					continue;
				}
				const koord3d kn3d(kn, welt->lookup_hgt(kn));
				grund_t* to = welt->lookup(kn3d);
				const weg_t* connecting_way = to ? to->get_weg(weg->get_waytype()) : NULL;
				const ribi_t::ribi connecting_ribi = connecting_way ? connecting_way->get_ribi() : ribi_t::all;
				if(route->is_contained(kn3d) || (ribi_t::is_single(ribi) && ribi_t::is_single(connecting_ribi)))
				{
					continue;
				}
				const grund_t* gr_neighbour = welt->lookup_kartenboden(kn);
				if(gr_neighbour && gr_neighbour->get_weg(weg->get_desc()->get_waytype()))
				{
					// This is a parallel way of the same type - reduce the forge cost.
					forge_cost *= welt->get_settings().get_parallel_ways_forge_cost_percentage(weg->get_waytype());
					forge_cost /= 100ll;
					break;
				}
			}
		}
		cost -= forge_cost;

		if((flags&has_way1)==0)
		{
			// new first way here, clear trees
			cost -= remove_trees();

			// Add the cost of buying the land, if appropriate.
			if(obj_bei(0) == NULL || obj_bei(0)->get_owner() != player)
			{
				// Only add the cost of the land if the player does not
				// already own this land.

				// get_land_value returns a *negative* value.
				cost += welt->get_land_value(pos);
			}

			// add
			weg->set_ribi(ribi);
			weg->set_pos(pos);
			objlist.add( weg );
			flags |= has_way1;
		}
		else
		{
			weg_t *other = (weg_t *)obj_bei(0);
			// another way will be added
			if(flags&has_way2) {
				dbg->fatal("grund_t::neuen_weg_bauen()","cannot built more than two ways on %i,%i,%i!",pos.x,pos.y,pos.z);
				return 0;
			}
			// add the way
			objlist.add( weg );
			weg->set_ribi(ribi);
			weg->set_pos(pos);
			flags |= has_way2;
			if(ist_uebergang()) {
				// no tram => crossing needed!
				waytype_t w2 =  other->get_waytype();
				const crossing_desc_t *cr_desc = crossing_logic_t::get_crossing( weg->get_waytype(), w2, weg->get_max_speed(), other->get_desc()->get_topspeed(), welt->get_timeline_year_month() );
				if(cr_desc==0) {
					dbg->fatal("crossing_t::crossing_t()","requested for waytypes %i and %i but nothing defined!", weg->get_waytype(), w2 );
				}
				crossing_t *cr = new crossing_t(obj_bei(0)->get_owner(), pos, cr_desc, ribi_t::is_straight_ns(get_weg(cr_desc->get_waytype(1))->get_ribi_unmasked()) );
				objlist.add( cr );
				cr->finish_rd();
			}
		}

		// Add a pavement to the new road if the old road also had a pavement.
		if (alter_weg && alter_weg->hat_gehweg()) {
			strasse_t *str = static_cast<strasse_t *>(weg);
			str->set_gehweg(true);
			weg->set_public_right_of_way();
		}

		// Add a pavement to roads adopted by the city.  This avoids the
		// "I deleted the road and replaced it, so now there's no pavement" phenomenon.
		bool city_adopts_this = weg->should_city_adopt_this(player);
		if (city_adopts_this)
		{
			// Add road and set speed limit.
			strasse_t *str = static_cast<strasse_t *>(weg);
			str->set_gehweg(true);
			weg->set_public_right_of_way();
		}
		else if(player && !is_water() && !city_adopts_this)
		{
			// Set the owner
			weg->set_owner(player);
			// Must call this here to ensure that the diagonal cost is
			// set as appropriate.
			// @author: jamespetts, Februrary 2010
			weg->finish_rd();
		}

		// may result in a crossing, but the wegebauer will recalc all images anyway
		weg->calc_image();
	}
	return cost;
}

sint32 grund_t::weg_entfernen(waytype_t wegtyp, bool ribi_rem)
{
	weg_t *weg = get_weg(wegtyp);
	if(weg!=NULL) {

		weg->mark_image_dirty(get_image(), 0);

#ifdef MULTI_THREAD
		if (env_t::networkmode)
		{
			// In network mode, we cannot have anything that alters a way running concurrently with
			// convoy path-finding because whether the convoy path-finder is called
			// on this tile of way before or after this function is indeterminate.
			if (!world()->get_first_step())
			{
				simthread_barrier_wait(&karte_t::step_convoys_barrier_external);
				welt->set_first_step(1);
			}
		}
#endif

		if(ribi_rem) {
			ribi_t::ribi ribi = weg->get_ribi_unmasked();
			grund_t *to;

			for(int r = 0; r < 4; r++) {
				if((ribi & ribi_t::nsew[r]) && get_neighbour(to, wegtyp, ribi_t::nsew[r])) {
					weg_t *weg2 = to->get_weg(wegtyp);
					if(weg2) {
						weg2->ribi_rem(ribi_t::backward(ribi_t::nsew[r]));
						to->calc_image();
					}
				}
			}
		}
		sint32 costs = (weg->get_desc()->get_value() / 2); // Costs for removal are half construction costs.
		weg->cleanup( NULL );
		delete weg;

		// delete the second way ...
		if(flags&has_way2) {
			flags &= ~has_way2;

			// reset speed limit/crossing info (maybe altered by crossing)
			// Not all ways (i.e. with styp==7) will imply crossings, so we have to check
			crossing_t* cr = find<crossing_t>(1);
			if(cr) {
				cr->cleanup(0);
				delete cr;
				// restore speed limit
				weg_t* w = (weg_t*)obj_bei(0);
				w->set_desc(w->get_desc(), true);
				w->count_sign();
			}
		}
		else {
			flags &= ~has_way1;
		}

		calc_image();
		reliefkarte_t::get_karte()->calc_map_pixel(get_pos().get_2d());

		return costs;
	}
	return 0;
}

bool grund_t::would_create_excessive_roads(int dx, int dy, road_network_plan_t &road_tiles)
{
	grund_t *gr[4];
	koord k = this->get_pos().get_2d();

	gr[0] = welt->lookup_kartenboden(k + koord(0,0));
	gr[1] = welt->lookup_kartenboden(k + koord(0,dy));
	gr[2] = welt->lookup_kartenboden(k + koord(dx,0));
	gr[3] = welt->lookup_kartenboden(k + koord(dx,dy));

	for (int i=1; i < 4; i++) {
		if (!gr[i]) {
			return false;
		}

		if (gr[i]->ist_bruecke() || gr[i]->ist_tunnel()) {
			return false;
		}

		wayobj_t *wo;
		if ((wo = gr[i]->get_wayobj(road_wt)) &&
		    wo->get_desc()->is_noise_barrier()) {
			return false;
		}

		if (gr[i]->get_leitung() != NULL) {
			return false;
		}

		koord k2 = gr[i]->get_pos().get_2d();

		if (road_tiles.is_contained(k2)) {
			if (road_tiles.get(k2)) {
				continue;
			} else {
				return false;
			}
		} else {
			weg_t *weg = gr[i]->get_weg(road_wt);
			if (weg) {
				continue;
			} else {
				return false;
			}
		}
	}

	return true;
}

/* avoid the pave-the-planet phenomenon: when extending diagonal
 * roads, straighten them by removing a tile of the diagonal road
 * which would otherwise result in a junction/roundabout:
 *
 *    . . . . .            . . . . .
 *    . . . . s            . . . . s
 *    . . X s s   becomes  . . s s s
 *    . . s s .            . . s . .
 *    . s s . .            . s s . .
 */

bool grund_t::remove_excessive_roads(int dx, int dy, road_network_plan_t &road_tiles)
{
	koord k = this->get_pos().get_2d();

	if (road_tiles.is_contained(k + koord(dx,dy)) &&
	    !road_tiles.get(k + koord(dx,dy))) {
		return false;
	}

	grund_t *gr[4];

	gr[0] = welt->lookup_kartenboden(k + koord(0,0));
	gr[1] = welt->lookup_kartenboden(k + koord(0,dy));
	gr[2] = welt->lookup_kartenboden(k + koord(dx,0));
	gr[3] = welt->lookup_kartenboden(k + koord(dx,dy));

	for (int i=1; i < 4; i++) {
		if (!gr[i]) {
			return false;
		}

		if (gr[i]->ist_bruecke() || gr[i]->ist_tunnel()) {
			return false;
		}

		if (gr[i]->get_leitung() != NULL) {
			return false;
		}

		wayobj_t *wo;
		if ((wo = gr[i]->get_wayobj(road_wt)) &&
		    wo->get_desc()->is_noise_barrier()) {
			return false;
		}

		koord k2 = gr[i]->get_pos().get_2d();

		if (road_tiles.is_contained(k2)) {
			if (road_tiles.get(k2)) {
				continue;
			} else {
				return false;
			}
		} else {
			weg_t *weg = gr[i]->get_weg(road_wt);
			if (weg) {
				continue;
			} else {
				return false;
			}
		}
	}

	koord k3 = k + koord(dx,dy);

	if (road_tiles.get(k3)) {
		return false;
	}

	if (gr[3]->removing_road_would_disconnect_city_building()) {
		return false;
	}

	if (gr[3]->removing_road_would_break_monument_loop()) {
		return false;
	}

	if (gr[3]->count_neighbouring_roads(road_tiles) >= 3) {
		return false;
	}

	road_tiles.set(k3, false);

	return true;
}

bool grund_t::would_create_excessive_roads(road_network_plan_t &road_tiles)
{
	return (would_create_excessive_roads(-1, -1, road_tiles) ||
		would_create_excessive_roads(-1,  1, road_tiles) ||
		would_create_excessive_roads( 1, -1, road_tiles) ||
		would_create_excessive_roads( 1,  1, road_tiles));
}

bool grund_t::would_create_excessive_roads()
{
	road_network_plan_t road_tiles;

	return would_create_excessive_roads(road_tiles);
}

bool grund_t::remove_excessive_roads(road_network_plan_t &road_tiles)
{
	return (remove_excessive_roads(-1, -1, road_tiles) ||
		remove_excessive_roads(-1,  1, road_tiles) ||
		remove_excessive_roads( 1, -1, road_tiles) ||
		remove_excessive_roads( 1,  1, road_tiles));
}

bool grund_t::remove_excessive_roads()
{
	road_network_plan_t road_tiles;
	bool ret = remove_excessive_roads(road_tiles);

	if (ret) {
		FOR(grund_t::road_network_plan_t, i, road_tiles) {
			koord k = i.key;
			grund_t *gr = welt->lookup_kartenboden(k);
			if (!i.value) {
				gr->weg_entfernen(road_wt, true);
			}
		}
	}

	return ret;
}

int grund_t::count_neighbouring_roads(road_network_plan_t &road_tiles)
{
	int count = 0;

	for (int i = 0; i < 4; i++) {
		koord k = ribi_t::nsew[i] + pos.get_2d();

		if (road_tiles.get(k)) {
			count++;
		} else {
			grund_t *gr = welt->lookup_kartenboden(k);

			if (gr && gr->hat_weg(road_wt)) {
				count++;
			}
		}
	}

	return count;
}

bool grund_t::fixup_road_network_plan(road_network_plan_t &road_tiles)
{
	FOR(road_network_plan_t, i, road_tiles) {
		if (i.value == true) {
			grund_t *gr = welt->lookup_kartenboden(i.key);

			while (gr->would_create_excessive_roads(road_tiles)) {
				if (!gr->remove_excessive_roads(road_tiles)) {
					return false;
				}
			}
		}
	}

	return true;
}

bool grund_t::removing_road_would_break_monument_loop()
{
	koord pos = get_pos().get_2d();

	for(int n = 0; n < 8; n ++)
	{
		const koord k = pos.neighbours[n] + pos;
		const grund_t* gr3 = welt->lookup_kartenboden(k);
		const gebaeude_t* gb = gr3 ? gr3->find<gebaeude_t>() : NULL;
		if(gb && gb->is_monument())
		{
			return true;
		}
	}

	return false;
}

bool grund_t::removing_road_would_disconnect_city_building()
{
	koord pos = get_pos().get_2d();
	// Players other than the public player cannot delete a road leaving no access to any city building.
	for(int n = 0; n < 8; n ++)
	{
		const koord k = pos.neighbours[n] + pos;
		const grund_t* gr3 = welt->lookup_kartenboden(k);
		const gebaeude_t* gb = gr3 ? gr3->find<gebaeude_t>() : NULL;
		if(gb && gb->get_owner() == NULL)
		{
			// This is a city building - check for other road connexion.
			bool unconnected_city_buildings = true;
			for(int m = 0; m < 8; m ++)
			{
				const koord kx = k.neighbours[m] + k;
				const grund_t* grx = welt->lookup_kartenboden(kx);
				if (grx == this)
				{
					// The road being deleted does not count - but others
					// at different heights DO count.
					continue;
				}
				const weg_t* w = grx ? grx->get_weg(road_wt) : NULL;
				wayobj_t* wo = grx ? grx->get_wayobj(road_wt) : NULL;
				if(w && (!wo || !wo->get_desc()->is_noise_barrier()) && w->get_ribi() && !ribi_t::is_single(w->get_ribi()))
				{
					// We must check that the road is itself connected to somewhere other
					// than the road that we are trying to delete.
					for(int q = 0; q < 4; q ++)
					{
						const koord ky = kx.nsew[q] + kx;
						const grund_t* gry = welt->lookup_kartenboden(ky);
						if (gry == this)
						{
							// The road being deleted does not count - but others
							// at different heights DO count.
							continue;
						}
						const weg_t* wy = gry ? gry->get_weg(road_wt) : NULL;
						if(wy)
						{
							if (wy->get_ribi() == 0) {
								// disconnected road tiles do not count
								continue;
							}
							if (ribi_t::is_single(wy->get_ribi())) {
								// only a single connection, but is it to the road we're trying to remove?
								grund_t *grz;
								if(gry->get_neighbour(grz, road_wt, wy->get_ribi())) {
									if (grz == this) {
										continue;
									}
								}
							}

							unconnected_city_buildings = false;
							break;
						}
					}
				}
			}

			if(unconnected_city_buildings)
			{
				return true;
			}
		}
	}

	return false;
}

class public_driver_t: public test_driver_t {
public:
	test_driver_t *other;

	static test_driver_t* apply(test_driver_t *test_driver) {
		public_driver_t *td2 = new public_driver_t();
		td2->other = test_driver;
		return td2;
	}

	~public_driver_t() {
		delete(other);
	}

	bool check_next_tile(const grund_t *gr) const {
		if (!other->check_next_tile(gr)) {
			return false;
		}


		if (get_waytype() == road_wt) {
			const roadsign_t *rs = gr->find<roadsign_t>();
			if (rs && rs->get_desc()->is_private_way() ) {
				return false;
			}

			const wayobj_t *wo = gr->get_wayobj(get_waytype());
			if (wo && wo->get_desc()->is_noise_barrier()) {
				return false;
			}
		}

		return true;
	}

	virtual ribi_t::ribi get_ribi(const grund_t* gr) const { return other->get_ribi(gr); }
	virtual waytype_t get_waytype() const { return other->get_waytype(); }
	virtual int get_cost(const grund_t *gr, const sint32 c, koord p) { return other->get_cost(gr,c,p); }
	virtual bool  is_target(const grund_t *gr,const grund_t *gr2) { return other-> is_target(gr,gr2); }
};

bool grund_t::removing_way_would_disrupt_public_right_of_way(waytype_t wt)
{
	weg_t *w = get_weg(wt);
	if (!w || !w->is_public_right_of_way()) {
		return false;
	}
	minivec_tpl<grund_t*> neighbouring_grounds(2);
	grund_t *gr = welt->lookup(pos);
	for(int n = 0; n < 4; n ++)
	{
		grund_t *to;
		if(w->get_waytype() == water_wt && gr->get_neighbour(to, invalid_wt, ribi_t::nsew[n] ) && to->get_typ() == grund_t::wasser) {
			neighbouring_grounds.append(to);
			continue;
		}

		if(gr->get_neighbour(to, w->get_waytype(), ribi_t::nsew[n])) {
			weg_t *way = to->get_weg(w->get_waytype());

			if(way && way->get_max_speed() > 0) {
				neighbouring_grounds.append(to);
			}
		}
	}

	if(neighbouring_grounds.get_count() > 1)
	{
		// It is necessary to do this to simulate the way not being there for testing purposes.
		grund_t* way_gr = welt->lookup(w->get_pos());
		koord3d start = koord3d::invalid;
		koord3d end;
		vehicle_desc_t diversion_check_type(w->get_waytype(), kmh_to_speed(w->get_max_speed()), vehicle_desc_t::petrol, w->get_max_axle_load());
		minivec_tpl<route_t> diversionary_routes;
		uint32 successful_diversions = 0;
		uint32 necessary_diversions = 0;
		FOR(minivec_tpl<grund_t*>, const& gr, neighbouring_grounds)
		{
			end = start;
			start = gr->get_pos();
			if(end != koord3d::invalid)
			{
				route_t diversionary_route;
				vehicle_t *diversion_checker = vehicle_builder_t::build(start, welt->get_public_player(), NULL, &diversion_check_type);
				diversion_checker->set_flag(obj_t::not_on_map);
				diversion_checker->set_owner(welt->get_public_player());
				test_driver_t *driver = diversion_checker;
				driver = public_driver_t::apply(driver);
				const way_desc_t* default_road = welt->get_city(w->get_pos().get_2d()) ? welt->get_settings().get_city_road_type(welt->get_timeline_year_month()) : welt->get_settings().get_intercity_road_type(welt->get_timeline_year_month());
				if(default_road == NULL) // If, for some reason, the default road is not defined
				{
					default_road = w->get_desc();
				}
				const uint32 default_road_axle_load = default_road->get_axle_load();
				const sint32 default_road_speed = default_road->get_topspeed();
				const uint32 max_axle_load = w->get_waytype() == road_wt ? min(default_road_axle_load, w->get_max_axle_load()) : w->get_max_axle_load();
				const sint32 max_speed = w->get_waytype() == road_wt ?  min(default_road_speed, w->get_max_speed()): w->get_max_speed();
				const uint32 bridge_weight_limit = gr->ist_bruecke() ? w->get_bridge_weight_limit() : 0;
				const uint32 bridge_weight = max_axle_load * (w->get_waytype() == road_wt ? 2 : 1);  // This is something of a fudge, but it is reasonable to assume that most road vehicles have 2 axles.
				if(diversionary_route.calc_route(welt, start, end, diversion_checker, w->get_max_speed(), max_axle_load, false, 0, welt->get_settings().get_max_diversion_tiles() * 100, bridge_weight) == route_t::valid_route)
				{
					// Only increment this counter if the ways were already connected.
					necessary_diversions ++;
				}
				const bool route_good = diversionary_route.calc_route(welt, start, end, diversion_checker, max_speed, max_axle_load, false, 0, welt->get_settings().get_max_diversion_tiles() * 100, bridge_weight_limit, w->get_pos()) == route_t::valid_route;
				if(route_good && (diversionary_route.get_count() < welt->get_settings().get_max_diversion_tiles()))
				{
					successful_diversions ++;
					diversionary_routes.append(diversionary_route);
				}
				delete diversion_checker;
			}
		}

		if(successful_diversions < necessary_diversions)
		{
			return true;
		}

		FOR(minivec_tpl<route_t>, const& diversionary_route, diversionary_routes)
		{
			for(int n = 1; n < diversionary_route.get_count()-1; n++)
			{
				// All diversionary routes must themselves be set as public rights of way.
				weg_t* way = welt->lookup(diversionary_route.at(n))->get_weg(w->get_waytype());
				if (way) {
					way->set_public_right_of_way();
				}
			}
		}
	}
	return false;
}

// this function is called many many times => make it as fast as possible
// i.e. no reverse lookup of ribis from koord
bool grund_t::get_neighbour(grund_t *&to, waytype_t type, ribi_t::ribi ribi) const
{
	// must be a single direction
	assert( ribi_t::is_single(ribi) );

	if(  type != invalid_wt  &&   (get_weg_ribi_unmasked(type) & ribi) == 0  ) {
		// no way on this tile in the given direction
		return false;
	}

	const planquadrat_t *plan = welt->access(pos.get_2d() + koord(ribi) );
	if(!plan) {
		return false;
	}
	const ribi_t::ribi back = ribi_t::reverse_single(ribi);

	// most common on empty ground => check this first
	if(  get_grund_hang() == slope_t::flat  &&  get_weg_hang() == slope_t::flat  ) {
		if(  grund_t *gr = plan->get_boden_in_hoehe( pos.z )  ) {
			if(  gr->get_grund_hang() == slope_t::flat  &&  gr->get_weg_hang() == slope_t::flat  ) {
				if(  type == invalid_wt  ||  (gr->get_weg_ribi_unmasked(type) & back)  ) {
					to = gr;
					return true;
				}
			}
		}
	}

	// most common on empty round => much faster this way
	const sint16 this_height = get_vmove(ribi);
	for( unsigned i=0;  i<plan->get_boden_count();  i++  ) {
		grund_t* gr = plan->get_boden_bei(i);
		if(gr->get_vmove(back)==this_height) {
			// test, if connected
			if(  type == invalid_wt  ||  (gr->get_weg_ribi_unmasked(type) & back)  ) {
				to = gr;
				return true;
			}
		}
	}
	return false;
}


int grund_t::get_max_speed() const
{
	int max = 0;
	if (weg_t const* const w = get_weg_nr(0)) {
		max = w->get_max_speed();
	}
	if (weg_t const* const w = get_weg_nr(1)) {
		max = min(max, w->get_max_speed());
	}
	return max;
}


bool grund_t::remove_everything_from_way(player_t* player, waytype_t wt, ribi_t::ribi rem)
{
	// check, if the way must be totally removed?
	weg_t *weg = get_weg(wt);
	if(weg) {
		waytype_t wt = weg->get_waytype();
		waytype_t finance_wt = weg->get_desc()->get_finance_waytype();
		const koord here = pos.get_2d();

		// stops
		if(flags&is_halt_flag  &&  (get_halt()->get_owner()==player  || player==welt->get_public_player())) {
			bool remove_halt = get_typ()!=boden;
			// remove only if there is no other way
			if(get_weg_nr(1)==NULL) {
				remove_halt = true;
			}
			else {
#ifdef delete_matching_stops
				// delete halts with the same waytype ... may lead to confusing / unexpected but completely logical results ;)
				gebaeude_t *gb = find<gebaeude_t>();
				if (gb) {
					waytype_t halt_wt = (waytype_t)gb->get_tile()->get_desc()->get_extra();
					if (halt_wt == wt || (wt==track_wt && halt_wt==tram_wt) || (wt==tram_wt && halt_wt==track_wt)) {
						remove_halt = true;
					}
				}
#else
				remove_halt = false;
#endif
			}
			if (remove_halt) {
				if (!haltestelle_t::remove(player, pos)) {
					return false;
				}
			}
		}
		// remove ribi from canals to sea level
		if(  wt == water_wt  &&  pos.z == welt->get_water_hgt( here )  &&  slope != slope_t::flat  ) {
			rem &= ~ribi_t::doubles(ribi_type(slope));
		}

		// remove ribi from canals to sea level
		if (wt==water_wt  &&  pos.z==welt->get_groundwater()  &&  slope!=slope_t::flat) {
			rem &= ~ribi_t::doubles(ribi_type(slope));
		}

		ribi_t::ribi add=(weg->get_ribi_unmasked()&rem);
		sint32 costs = 0;

		for(  sint16 i=get_top();  i>=0;  i--  ) {
			// we need to delete backwards, since we might miss things otherwise
			if(  i>=get_top()  ) {
				continue;
			}

			obj_t *obj=obj_bei((uint8)i);
			// do not delete ways
			if(  obj->get_typ()==obj_t::way  ) {
				continue;
			}
			if (roadsign_t* const sign = obj_cast<roadsign_t>(obj)) {
				// roadsigns: check dir: dirs changed => delete
				if (sign->get_desc()->get_wtyp() == wt && (sign->get_dir() & ~add) != 0) {
					costs -= sign->get_desc()->get_value();
					delete sign;
				}
			} else if (signal_t* const signal = obj_cast<signal_t>(obj)) {
				// signal: not on crossings => remove all
				if (signal->get_desc()->get_wtyp() == wt) {
					costs -= signal->get_desc()->get_value();
					delete signal;
				}
			} else if (wayobj_t* const wayobj = obj_cast<wayobj_t>(obj)) {
				// wayobj: check dir
				if (add == ribi_t::none && wayobj->get_desc()->get_wtyp() == wt) {
					uint8 new_dir=wayobj->get_dir()&add;
					if(new_dir) {
						// just change dir
						wayobj->set_dir(new_dir);
					}
					else {
						costs -= wayobj->get_desc()->get_value();
						delete wayobj;
					}
				}
			} else if (private_car_t* const citycar = obj_cast<private_car_t>(obj)) {
				// citycar: just delete
				if (wt == road_wt) delete citycar;
			} else if (pedestrian_t* const pedestrian = obj_cast<pedestrian_t>(obj)) {
				// pedestrians: just delete
				if (wt == road_wt) delete pedestrian;
			} else if (tunnel_t* const tunnel = obj_cast<tunnel_t>(obj)) {
				// remove tunnel portal, if not the last tile ...
				// must be done before weg_entfernen() to get maintenance right
				uint8 wt = tunnel->get_desc()->get_waytype();
				if (weg->get_waytype()==wt) {
					if((flags&has_way2)==0) {
						if (add==ribi_t::none) {
							// last way was belonging to this tunnel
							tunnel->cleanup(player);
							delete tunnel;
						}
					}
					else {
						// we must leave the way to prevent destroying the other one
						add |= get_weg_nr(1)->get_ribi_unmasked();
						weg->calc_image();
					}
				}
			}
		}

		// need to remove railblocks to recalculate connections
		// remove all ways or just some?
		if(add==ribi_t::none) {
			player_t* owner = weg->get_owner();
			koord3d pos = weg->get_pos();
			costs -= weg_entfernen(wt, true);
			if(owner == player)
			{
				// Need to sell the land on which the way is situated: refund the land value.
				// Note that get_land_value() produces a negative number.
				costs -= welt->get_land_value(pos);
			}
			if(flags&is_kartenboden) {
				// remove ribis from sea tiles
				if(  wt == water_wt  &&  pos.z == welt->get_water_hgt( here )  &&  slope != slope_t::flat  ) {
					grund_t *gr = welt->lookup_kartenboden(here - ribi_type(slope));
					if (gr  &&  gr->is_water()) {
						gr->calc_image(); // to recalculate ribis
					}
				}
				// make tunnel portals to normal ground
				if (get_typ()==tunnelboden  &&  (flags&has_way1)==0) {
					// remove remaining objs
					obj_loesche_alle( player );
					// set to normal ground
					welt->access(here)->kartenboden_setzen( new boden_t( pos, slope ) );
					// now this is already deleted !
				}
			}
		}
		else {
DBG_MESSAGE("tool_wayremover()","change remaining way to ribi %d",add);
			// something will remain, we just change ribis
			weg->set_ribi(add);
			calc_image();
		}
		// we have to pay?
		if(costs) {
			player_t::book_construction_costs(player, costs, here, finance_wt);
		}
	}
	return true;
}


wayobj_t *grund_t::get_wayobj( waytype_t wt ) const
{
	waytype_t wt1 = ( wt == tram_wt ) ? track_wt : wt;

	// since there might be more than one, we have to iterate through all of them
	for(  uint8 i = 0;  i < get_top();  i++  ) {
		obj_t *obj = obj_bei(i);
		if (wayobj_t* const wayobj = obj_cast<wayobj_t>(obj)) {
			waytype_t wt2 = wayobj->get_desc()->get_wtyp();
			if(  wt2 == tram_wt  ) {
				wt2 = track_wt;
			}
			if(  wt1 == wt2  ) {
				return wayobj;
			}
		}
	}
	return NULL;
}
