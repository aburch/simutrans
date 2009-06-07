/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include <math.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"
#include "../player/simplay.h"
#include "../simtools.h"
#include "../simtypes.h"

#include "../boden/grund.h"

#include "../besch/baum_besch.h"

#include "../dings/groundobj.h"

#include "../utils/cbuffer_t.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/tabfile.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/freelist.h"


#include "baum.h"

static const uint8 baum_bild_alter[12] =
{
	0,1,2,3,3,3,3,3,3,4,4,4
};

/******************************** static stuff for forest rules ****************************************************************/

// at first deafault values for forest creation will be set
// Base forest size - minimal size of forest - map independent
uint8 baum_t::forest_base_size = 36;

// Map size divisor - smaller it is the larger are individual forests
uint8 baum_t::forest_map_size_divisor = 38;

// Forest count divisor - smaller it is, the more forest are generated
uint8 baum_t::forest_count_divisor = 16;

// Forest boundary sharpenss: 0 - perfectly sharp boundaries, 20 - very blurred
uint8 baum_t::forest_boundary_blur = 6;

// Forest boundary thickness  - determines how thick will the boundary line be
uint8 baum_t::forest_boundary_thickness = 2;

// Determins how often are spare trees going to be planted (works inversly)
uint16 baum_t::forest_inverse_spare_tree_density = 5;

// Number of trees on square 2 - minimal usable, 3 good, 5 very nice looking
uint8 baum_t::max_no_of_trees_on_square = 3;

// bit set, if this climate is to be covered with trees entirely
uint16 baum_t::tree_climates = 0;

// bit set, if this climate is to be void of random trees
uint16 baum_t::no_tree_climates = 0;



/**
 * Reads forest configuration data
 * @author prissi
 */
bool
baum_t::forestrules_init(cstring_t objfilename)
{
	tabfile_t forestconf;
	// first take user data, then user global data
	cstring_t user_dir=umgebung_t::user_dir;
	if (!forestconf.open(user_dir+"forestrules.tab")) {
		if (!forestconf.open(objfilename+"config/forestrules.tab")) {
			dbg->warning("baum_t::forestrules_init()", "Can't read forestrules.tab" );
			return false;
		}
	}

	tabfileobj_t contents;
	forestconf.read(contents);

	baum_t::forest_base_size = contents.get_int("forest_base_size", baum_t::forest_base_size );
	baum_t::forest_map_size_divisor = contents.get_int("forest_map_size_divisor", baum_t::forest_map_size_divisor );
	baum_t::forest_count_divisor = contents.get_int("forest_count_divisor", baum_t::forest_count_divisor );
	baum_t::forest_boundary_blur = contents.get_int("forest_boundary_blur", baum_t::forest_boundary_blur );
	baum_t::forest_boundary_thickness = contents.get_int("forest_boundary_thickness", baum_t::forest_boundary_thickness );
	baum_t::forest_inverse_spare_tree_density = contents.get_int("forest_inverse_spare_tree_density", baum_t::forest_inverse_spare_tree_density );
	baum_t::max_no_of_trees_on_square = contents.get_int("max_no_of_trees_on_square", baum_t::max_no_of_trees_on_square );
	baum_t::tree_climates = contents.get_int("tree_climates", baum_t::tree_climates );
	baum_t::no_tree_climates = contents.get_int("no_tree_climates", baum_t::no_tree_climates );

	return true;
}



// distributes trees on a map
void
baum_t::distribute_trees(karte_t *welt, int dichte)
{
	// now we can proceed to tree palnting routine itself
	// best forests results are produced if forest size is tied to map size -
	// but there is some nonlinearity to ensure good forests on small maps
	const unsigned t_forest_size = (unsigned)pow( welt->get_groesse_max()>>7 , 0.5)*forest_base_size + (welt->get_groesse_max()/forest_map_size_divisor);
	const uint8 c_forest_count = welt->get_groesse_max()/forest_count_divisor;

DBG_MESSAGE("verteile_baeume()","creating %i forest",c_forest_count);
	for (uint8 c1 = 0 ; c1 < c_forest_count ; c1++) {
		create_forest( welt, koord( simrand(welt->get_groesse_x()), simrand(welt->get_groesse_y()) ), koord( (t_forest_size*(1+simrand(2))), (t_forest_size*(1+simrand(2))) ) );
	}

	fill_trees(welt, dichte);
}




/*************************** first the static function for the baum_t and baum_besch_t administration ***************/

/*
 * Diese Tabelle ermöglicht das Auffinden dient zur Auswahl eines Baumtypen
 */
vector_tpl<const baum_besch_t *> baum_t::baum_typen(0);

/*
 * Diese Tabelle ermöglicht das Auffinden einer Beschreibung durch ihren Namen
 */
stringhashtable_tpl<const baum_besch_t *> baum_t::besch_names;

// total number of trees
// the same for a certain climate
int baum_t::get_anzahl_besch(climate cl)
{
	uint16 total_number=0;
	for( uint32 i=0;  i<baum_typen.get_count();  i++  ) {
		if (baum_typen[i]->is_allowed_climate(cl)) {
			total_number ++;
		}
	}
	return total_number;
}



/**
 * tree planting function - it takes care of checking suitability of area
 */
bool baum_t::plant_tree_on_coordinate(karte_t * welt, koord pos, const uint8 maximum_count)
{
	const planquadrat_t *plan = welt->lookup(pos);
	if(plan) {
		grund_t * gr = plan->get_kartenboden();
		if(gr!=NULL  &&
			get_anzahl_besch(welt->get_climate(gr->get_pos().z))>0  &&
			gr->ist_natur()  &&
			gr->get_top()<maximum_count)
		{
			if(gr->get_top()>0) {
				switch(gr->obj_bei(0)->get_typ()) {
					case ding_t::wolke:
					case ding_t::aircraft:
					case ding_t::baum:
					case ding_t::leitung:
					case ding_t::label:
					case ding_t::zeiger:
						// ok to built here
						break;
					case ding_t::groundobj:
						if(((groundobj_t *)(gr->obj_bei(0)))->get_besch()->can_built_trees_here()) {
							break;
						}
						// leave these (and all other empty)
					default:
						return false;
				}
			}
			gr->obj_add( new baum_t(welt, gr->get_pos()) ); //plants the tree
			return true; //tree was planted - currently unused value is not checked
		}
	}
	return false;
}



/**
 * tree planting function - it takes care of checking suitability of area
 */
bool baum_t::plant_tree_on_coordinate(karte_t * welt, koord pos, const baum_besch_t *besch, const bool check_climate, const bool random_age )
{
	// none there
	if(  besch_names.empty()  ) {
		return false;
	}
	grund_t *gr = welt->lookup_kartenboden(pos);
	if(gr) {
		if( gr->ist_natur()  &&
			gr->get_top()<10  &&
			(!check_climate  ||  besch->is_allowed_climate(welt->get_climate(gr->get_hoehe())))
			)
		{
			if(gr->get_top()>0) {
				switch(gr->obj_bei(0)->get_typ()) {
					case ding_t::wolke:
					case ding_t::aircraft:
					case ding_t::baum:
					case ding_t::leitung:
					case ding_t::label:
					case ding_t::zeiger:
						// ok to built here
						break;
					case ding_t::groundobj:
						if(((groundobj_t *)(gr->obj_bei(0)))->get_besch()->can_built_trees_here()) {
							break;
						}
						// leave these (and all other empty)
					default:
						return false;
				}
			}
			baum_t *b = new baum_t(welt, gr->get_pos(), besch); //plants the tree
			if(random_age) {
				b->geburt = welt->get_current_month() - simrand(400);
			}
			gr->obj_add( b );
			return true; //tree was planted - currently unused value is not checked
		}
	}
	return false;
}



uint32 baum_t::create_forest(karte_t *welt, koord new_center, koord wh )
{
	// none there
	if(  besch_names.empty()  ) {
		return 0;
	}
	const sint16 xpos_f = new_center.x;
	const sint16 ypos_f = new_center.y;
	uint32 number_of_new_trees = 0;
	for( sint16 j = 0; j < wh.x; j++) {
		for( sint16 i = 0; i < wh.y; i++) {

			const sint32 x_tree_pos = (j-(wh.x>>1));
			const sint32 y_tree_pos = (i-(wh.y>>1));

			const uint32 distance = 1 + ((uint32) sqrt( (double)(x_tree_pos*x_tree_pos*(wh.y*wh.y) + y_tree_pos*y_tree_pos*(wh.x*wh.x))));
			const uint32 tree_probability = ( 8 * (uint32)((wh.x*wh.x)+(wh.y*wh.y)) ) / distance;

			for (uint8 c2 = 0 ; c2<max_no_of_trees_on_square; c2++) {
				const uint32 rating = simrand(10) + 38 + c2*2;
				if (rating < tree_probability ) {
					const koord pos( (sint16)(xpos_f+x_tree_pos), (sint16)(ypos_f+y_tree_pos));
					baum_t::plant_tree_on_coordinate(welt, pos, 3 );
					number_of_new_trees ++;
				}
			}
		}
	}
	return number_of_new_trees;
}


void baum_t::fill_trees(karte_t *welt, int dichte)
{
	// none there
	if(  besch_names.empty()  ) {
		return;
	}
DBG_MESSAGE("verteile_baeume()","distributing single trees");
	koord pos;
	for(pos.y=0;pos.y<welt->get_groesse_y(); pos.y++) {
		for(pos.x=0; pos.x<welt->get_groesse_x(); pos.x++) {
			grund_t *gr = welt->lookup_kartenboden(pos);
			if(gr->get_top() == 0  &&  gr->get_typ() == grund_t::boden)  {
				// plant spare trees, (those with low preffered density) or in an entirely tree climate
				uint16 cl = 1<<welt->get_climate(gr->get_hoehe());
				if( (cl & no_tree_climates)==0  &&  (  (cl & tree_climates)!=0  ||  simrand(forest_inverse_spare_tree_density*dichte) < 100  )  ) {
					plant_tree_on_coordinate(welt, pos, 1);
				}
			}
		}
	}
}



bool baum_t::alles_geladen()
{
	if (besch_names.empty()) {
		DBG_MESSAGE("baum_t", "No trees found - feature disabled");
		baum_typen.append( NULL );
	}
	else {
		stringhashtable_iterator_tpl<const baum_besch_t*> iter(besch_names);
		while(  iter.next()  ) {
			baum_typen.append( iter.get_current_value() );
		}
	}
	return true;
}



bool baum_t::register_besch(baum_besch_t *besch)
{
	// avoid duplicates with same name
	if(besch_names.remove(besch->get_name())) {
		dbg->warning( "baum_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
	}
	besch_names.put(besch->get_name(), besch );
	return true;
}



// calculates tree position on a tile
// takes care of slopes
void baum_t::calc_off()
{
	int liob;
	int reob;
	switch (welt->lookup( get_pos().get_2d())->get_kartenboden()->get_grund_hang() ) {
		case 0:
			liob=simrand(30)-15;
			reob=simrand(30)-15;
			set_xoff( reob - liob  );
			set_yoff( (reob + liob)/2 );
			break;

		case 1:
		case 4:
		case 5:
		case 8:
		case 9:
		case 12:
		case 13:
			liob=simrand(16)-8;
			reob=simrand(16)-8;
			set_xoff( reob - liob  );
			set_yoff( reob + liob );
			break;

		case 2:
		case 3:
		case 6:
		case 7:
		case 10:
		case 11:
		case 14:
		case 15:
			liob=simrand(TILE_STEPS)-(TILE_STEPS/2);
			reob=simrand(TILE_STEPS)-(TILE_STEPS/2);
			set_xoff( reob + liob  );
			set_yoff( -(10*TILE_STEPS/16)-(reob - liob)/2 );
			break;
	}
}



void
baum_t::calc_bild()
{
	// alter/2048 is the age of the tree
	const baum_besch_t *besch=get_besch();
	const sint16 seasons = besch->get_seasons();
	season=0;

	if(seasons>1) {
		// two possibilities
		if(besch->get_seasons()<4) {
			// only summer and winter
			season = welt->get_snowline()<=get_pos().z;
		}
		else {
			// summer autumn winter spring
			season = welt->get_jahreszeit();
			if(welt->get_snowline()<=get_pos().z) {
				// change to winter
				if(seasons==5) {
					// snowy winter graphics (3 or 5)
					season = 4;
				}
				else {
					// no special winter graphics
					season = 2;
				}
			}
			else if(welt->get_snowline()<=get_pos().z+Z_TILE_STEP  &&  season==0) {
				// snowline crossing in summer
				// so at least some weeks spring/autumn
				season = welt->get_last_month() <=5 ? 3 : 1;
			}
		}
	}
}



image_id baum_t::get_bild() const
{
	// alter/2048 is the age of the tree
	if(umgebung_t::hide_trees) {
		return umgebung_t::hide_with_transparency ? IMG_LEER : get_besch()->get_bild_nr( season, 0 );
		// we need the real age for transparency or real image
	}
	uint8 baum_alter = baum_bild_alter[min((welt->get_current_month() - geburt)>>6, 11u)];
	return get_besch()->get_bild_nr( season, baum_alter );
}



// image which transparent outline is used
image_id
baum_t::get_outline_bild() const
{
	uint8 baum_alter = baum_bild_alter[min((welt->get_current_month() - geburt)>>6, 11u)];
	return get_besch()->get_bild_nr( season, baum_alter );
}



/* also checks for distribution values
 * @author prissi
 */
uint16 baum_t::random_tree_for_climate_intern(climate cl)
{
	int weight = 0;

	for( uint32 i=0;  i<baum_typen.get_count();  i++  ) {
		if (baum_typen[i]->is_allowed_climate(cl)) {
			weight += baum_typen[i]->get_distribution_weight();
		}
	}

	// now weight their distribution
	if (weight > 0) {
		const int w=simrand(weight);
		weight = 0;
		for( uint32 i=0; i<baum_typen.get_count();  i++  ) {
			if (baum_typen[i]->is_allowed_climate(cl)) {
				weight += baum_typen[i]->get_distribution_weight();
				if(weight>=w) {
					return i;
				}
			}
		}
	}
	return 0xFFFF;
}



baum_t::baum_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	season = 0;
	geburt = welt->get_current_month();
	baumtype = 0;
	rdwr(file);
}



baum_t::baum_t(karte_t *welt, koord3d pos) : ding_t(welt, pos)
{
	// Hajo: auch aeltere Baeume erzeugen
	geburt = welt->get_current_month() - simrand(400);
	baumtype = random_tree_for_climate_intern(welt->get_climate(pos.z));
	season = 0;
	calc_off();
	calc_bild();
}



baum_t::baum_t(karte_t *welt, koord3d pos, uint16 type) : ding_t(welt, pos)
{
	geburt = welt->get_current_month();
	baumtype = type;
	season = 0;
	calc_off();
	calc_bild();
}

baum_t::baum_t(karte_t *welt, koord3d pos, const baum_besch_t *besch) : ding_t(welt, pos)
{
	geburt = welt->get_current_month();
	baumtype = baum_typen.index_of(besch);
	season = 0;
	calc_off();
	calc_bild();
}


void
baum_t::saee_baum()
{
	// spawn a new tree in an area 5x5 tiles around
	// the area for normal new tree planting is slightly more restricted, square of 9x9 was too much
	const koord k = get_pos().get_2d() + koord(simrand(5)-2, simrand(5)-2);
	const planquadrat_t* p = welt->lookup(k);
	if (p) {
		grund_t *bd = p->get_kartenboden();
		if(	bd!=NULL  &&
			get_besch()->is_allowed_climate(welt->get_climate(bd->get_pos().z))  &&
			bd->ist_natur()  &&
			bd->get_top()<max_no_of_trees_on_square)
		{
			bd->obj_add( new baum_t(welt, bd->get_pos(), baumtype) );
		}
	}
}



/* we should be as fast as possible for this, because trees are nearly the most common object on a map */
bool
baum_t::check_season(long month)
{
	// take care of birth/death and seasons
	const long alter = (month - geburt);
	calc_bild();
	if(alter==512) {
		// only in this month a tree can span new trees
		// only 1-3 trees will be planted....
		const uint8 c_plant_tree_max = 1+simrand(3);
		for(uint8 c_temp=0 ;  c_temp<c_plant_tree_max;  c_temp++ ) {
			saee_baum();
		}
		// we make the tree a month older to avoid second spawning
		geburt = geburt-1;
	}
	// tree will die after 704 month (i.e. 58 years 8 month)
	if(alter>=704) {
		mark_image_dirty( get_bild(), 0 );
		return false;
	}
	return true;
}



void
baum_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "baum_t" );

	ding_t::rdwr(file);

	sint32 alter = (welt->get_current_month() - geburt)<<18;
	file->rdwr_long(alter, "\n");

	// after loading, calculate new
	geburt = welt->get_current_month() - (alter>>18);

	if(file->is_loading()) {
		char buf[128];
		file->rdwr_str(buf, 128);
		const baum_besch_t *besch = besch_names.get(buf);
		if(  baum_typen.is_contained(besch)  ) {
			baumtype = baum_typen.index_of( besch );
		}
		else {
			// replace with random tree
			baumtype = simrand(baum_typen.get_count());
		}
	}
	else {
		const char *c = get_besch()->get_name();
		file->rdwr_str(c);
	}
}



/**
 * Öffnet ein neues Beobachtungsfenster für das Objekt.
 * @author Hj. Malthaner
 */
void baum_t::zeige_info()
{
	if(umgebung_t::tree_info) {
		ding_t::zeige_info();
	}
}



/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void baum_t::info(cbuffer_t & buf) const
{
	ding_t::info(buf);

	buf.append("\n");
	buf.append(translator::translate(get_besch()->get_name()));
	buf.append("\n");
	buf.append(welt->get_current_month() - geburt);
	buf.append(" ");
	buf.append(translator::translate("Monate alt"));
}



void
baum_t::entferne(spieler_t *sp) //"remove" (Babelfish)
{
	spieler_t::accounting(sp, welt->get_einstellungen()->cst_remove_tree, get_pos().get_2d(), COST_CONSTRUCTION);
	mark_image_dirty( get_bild(), 0 );
}



void *
baum_t::operator new(size_t /*s*/)
{
	return freelist_t::gimme_node(sizeof(baum_t));
}



void
baum_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(baum_t),p);
}
