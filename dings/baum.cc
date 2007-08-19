/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>
#include <math.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"
#include "../simplay.h"
#include "../simmem.h"
#include "../simtools.h"
#include "../simtypes.h"

#include "../boden/grund.h"

#include "../besch/baum_besch.h"

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

	return true;
}



// distributes trees on a map
void
baum_t::distribute_trees(karte_t *welt, int dichte)
{
	// now we can proceed to tree palnting routine itself
	// best forests results are produced if forest size is tied to map size -
	// but there is some nonlinearity to ensure good forests on small maps
	const unsigned t_forest_size = (unsigned)pow( welt->gib_groesse_max()>>7 , 0.5)*forest_base_size + (welt->gib_groesse_max()/forest_map_size_divisor);
	const uint8 c_forest_count = welt->gib_groesse_max()/forest_count_divisor;
	unsigned  x_tree_pos, y_tree_pos, distance, tree_probability;
	uint8 c2;

DBG_MESSAGE("verteile_baeume()","creating %i forest",c_forest_count);
	for (uint8 c1 = 0 ; c1 < c_forest_count ; c1++) {
		const unsigned xpos_f = simrand(welt->gib_groesse_x());
		const unsigned ypos_f = simrand(welt->gib_groesse_y());
		const unsigned c_coef_x = 1+simrand(2);
		const unsigned c_coef_y = 1+simrand(2);
		const unsigned x_forest_boundary = t_forest_size*c_coef_x;
		const unsigned y_forest_boundary = t_forest_size*c_coef_y;
		unsigned i, j;

		for(j = 0; j < x_forest_boundary; j++) {
			for(i = 0; i < y_forest_boundary; i++) {

				x_tree_pos = (j-(t_forest_size>>1)); // >>1 works like 2 but is faster
				y_tree_pos = (i-(t_forest_size>>1));

				distance = 1 + ((int) sqrt( (double)(x_tree_pos*x_tree_pos/c_coef_x + y_tree_pos*y_tree_pos/c_coef_y)));

				tree_probability = ( 32 * (t_forest_size / 2) ) / distance;

				for (c2 = 0 ; c2<max_no_of_trees_on_square; c2++) {
					const unsigned rating = simrand(forest_boundary_blur) + 38 + c2*forest_boundary_thickness;
					if (rating < tree_probability ) {
						const koord pos( (sint16)(xpos_f+x_tree_pos), (sint16)(ypos_f+y_tree_pos));
						plant_tree_on_coordinate(welt, pos, max_no_of_trees_on_square );
					}
				}
			}
		}
	}

DBG_MESSAGE("verteile_baeume()","distributing single trees");
	koord pos;
	for(pos.y=0;pos.y<welt->gib_groesse_y(); pos.y++) {
		for(pos.x=0; pos.x<welt->gib_groesse_x(); pos.x++) {
			//plant spare trees, (those with low preffered density)
			if(simrand(forest_inverse_spare_tree_density*dichte) < 100) {
				plant_tree_on_coordinate(welt, pos, 1);
			}
		}
	}
}




/*************************** first the static function for the baum_t and baum_besch_t administration ***************/

/*
 * Diese Tabelle ermöglicht das Auffinden dient zur Auswahl eines Baumtypen
 */
vector_tpl<const baum_besch_t *> baum_t::baum_typen(0);

/*
 * Diese Tabelle ermöglicht das Auffinden einer Beschreibung durch ihren Namen
 */
stringhashtable_tpl<uint32> baum_t::besch_names;

// total number of trees
// the same for a certain climate
int baum_t::gib_anzahl_besch(climate cl)
{
	uint16 total_number=0;
	for( unsigned i=0;  i<baum_typen.get_count();  i++  ) {
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
		grund_t * gr = plan->gib_kartenboden();
		if(gr!=NULL  &&
			gib_anzahl_besch(welt->get_climate(gr->gib_pos().z))>0  &&
			gr->ist_natur()  &&
			gr->gib_top()<maximum_count)
		{
			gr->obj_add( new baum_t(welt, gr->gib_pos()) ); //plants the tree
			return true; //tree was planted - currently unused value is not checked
		}
	}
	return false;
}



bool
baum_t::alles_geladen()
{
	if (besch_names.empty()) {
		DBG_MESSAGE("baum_t", "No trees found - feature disabled");
	}
	return true;
}



bool
baum_t::register_besch(baum_besch_t *besch)
{
	baum_typen.append(besch,4);
	besch_names.put(besch->gib_name(), baum_typen.get_count() );
	return true;
}



// calculates tree position on a tile
// takes care of slopes
void
baum_t::calc_off()
{
	int liob;
	int reob;
	switch (welt->lookup( gib_pos().gib_2d())->gib_kartenboden()->gib_grund_hang() ) {
		case 0:
			liob=simrand(30)-15;
			reob=simrand(30)-15;
			setze_xoff( reob - liob  );
			setze_yoff( (reob + liob)/2 );
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
			setze_xoff( reob - liob  );
			setze_yoff( reob + liob );
			break;

		case 2:
		case 3:
		case 6:
		case 7:
		case 10:
		case 11:
		case 14:
		case 15:
			liob=simrand(16)-8;
			reob=simrand(16)-8;
			setze_xoff( reob + liob  );
			setze_yoff(-10-(reob - liob)/2 );
			break;
	}
}



void
baum_t::calc_bild()
{
	// alter/2048 is the age of the tree
	const baum_besch_t *besch=gib_besch();
	const sint16 seasons = besch->gib_seasons();
	season=0;

	if(seasons>1) {
		// two possibilities
		if(besch->gib_seasons()<4) {
			// only summer and winter
			season = welt->get_snowline()<=gib_pos().z;
		}
		else {
			// summer autumn winter spring
			season = welt->gib_jahreszeit();
			if(welt->get_snowline()<=gib_pos().z) {
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
			else if(welt->get_snowline()<=gib_pos().z+Z_TILE_STEP  &&  season==0) {
				// snowline crossing in summer
				// so at least some weeks spring/autumn
				season = welt->get_last_month() <=5 ? 3 : 1;
			}
		}
	}
}



image_id
baum_t::gib_bild() const
{
	// alter/2048 is the age of the tree
	if(umgebung_t::hide_trees) {
		return umgebung_t::hide_with_transparency ? IMG_LEER : gib_besch()->gib_bild( season, 0 )->gib_nummer();
		// we need the real age for transparency or real image
	}
	uint8 baum_alter = baum_bild_alter[min((welt->get_current_month() - geburt)>>6, 11u)];
	return gib_besch()->gib_bild( season, baum_alter )->gib_nummer();
}



// image which transparent outline is used
image_id
baum_t::gib_outline_bild() const
{
	uint8 baum_alter = baum_bild_alter[min((welt->get_current_month() - geburt)>>6, 11u)];
	return gib_besch()->gib_bild( season, baum_alter )->gib_nummer();
}



/* also checks for distribution values
 * @author prissi
 */
const uint16
baum_t::random_tree_for_climate(climate cl)
{
	int weight = 0;

	for( unsigned i=0;  i<baum_typen.get_count();  i++  ) {
		if (baum_typen[i]->is_allowed_climate(cl)) {
			weight += baum_typen[i]->gib_distribution_weight();
		}
	}

	// now weight their distribution
	if (weight > 0) {
		const int w=simrand(weight);
		weight = 0;
		for( unsigned i=0; i<baum_typen.get_count();  i++  ) {
			if (baum_typen[i]->is_allowed_climate(cl)) {
				weight += baum_typen[i]->gib_distribution_weight();
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
	rdwr(file);
	if(gib_besch()) {
		calc_bild();
	}
}



baum_t::baum_t(karte_t *welt, koord3d pos) : ding_t(welt, pos)
{
	// Hajo: auch aeltere Baeume erzeugen
	geburt = welt->get_current_month() - simrand(400);

	baumtype = random_tree_for_climate(welt->get_climate(pos.z));

	calc_off();
	calc_bild();
}



baum_t::baum_t(karte_t *welt, koord3d pos, uint16 type) : ding_t(welt, pos)
{
	geburt = welt->get_current_month();

	baumtype = type;
	calc_off();
	calc_bild();
}



void
baum_t::saee_baum()
{
	// spawn a new tree in an area 5x5 tiles around
	// the area for normal new tree planting is slightly more restricted, square of 9x9 was too much
	const koord k = gib_pos().gib_2d() + koord(simrand(5)-2, simrand(5)-2);
	const planquadrat_t* p = welt->lookup(k);
	if (p) {
		grund_t *bd = p->gib_kartenboden();
		if(	bd!=NULL  &&
			gib_besch()->is_allowed_climate(welt->get_climate(bd->gib_pos().z))  &&
			bd->ist_natur()  &&
			bd->gib_top()<max_no_of_trees_on_square)
		{
			bd->obj_add( new baum_t(welt, bd->gib_pos(), baumtype) );
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
	return alter<704;
}



void
baum_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);

	sint32 alter = (welt->get_current_month() - geburt)<<18;
	file->rdwr_long(alter, "\n");

	// after loading, calculate new
	geburt = welt->get_current_month() - (alter>>18);

	if(file->is_saving()) {
		const char *s = gib_besch()->gib_name();
		file->rdwr_str(s, "N");
	}

	if(file->is_loading()) {
		char bname[128];
		file->rd_str_into(bname, "N");

		baumtype = 0;
		baumtype = besch_names.get(bname);
		if(baumtype==0) {
			// replace with random tree
			baumtype = simrand(baum_typen.get_count())+1;
		}
		baumtype --;
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
	buf.append(translator::translate(gib_besch()->gib_name()));
	buf.append("\n");
	buf.append(welt->get_current_month() - geburt);
	buf.append(" ");
	buf.append(translator::translate("Monate alt"));
}



void
baum_t::entferne(spieler_t *sp)
{
	if(sp != NULL) {
		sp->buche(umgebung_t::cst_remove_tree, gib_pos().gib_2d(), COST_CONSTRUCTION);
	}
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
