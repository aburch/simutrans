/*
 * Brueckenteile (sichtbar)
 *
 * Hj. Malthaner
 */

#include "../simworld.h"
#include "../simtypes.h"
#include "../simdings.h"
#include "../boden/grund.h"
#include "../player/simplay.h"
#include "../simimg.h"
#include "../simmem.h"
#include "../bauer/brueckenbauer.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"

#include "bruecke.h"



bruecke_t::bruecke_t(karte_t* const welt, loadsave_t* const file) : ding_no_info_t(welt)
{
	rdwr(file);
}


bruecke_t::bruecke_t(karte_t *welt, koord3d pos, spieler_t *sp, const bruecke_besch_t *besch, bruecke_besch_t::img_t img) :
 ding_no_info_t(welt, pos)
{
	this->besch = besch;
	this->img = img;
	set_besitzer( sp );
	spieler_t::book_construction_costs( get_besitzer(), -besch->get_preis(), get_pos().get_2d(), besch->get_waytype());
}


void bruecke_t::calc_bild()
{
	grund_t *gr=welt->lookup(get_pos());
	if(gr) {
		// if we are on the bridge, put the image into the ground, so we can have two ways ...
		if(  weg_t *weg0 = gr->get_weg_nr(0)  ) {
#if MULTI_THREAD>1
			weg0->lock_mutex();
#endif
			if(img>=bruecke_besch_t::N_Start  &&  img<=bruecke_besch_t::W_Start) {
				// must take the upper value for the start of the bridge
				weg0->set_bild(besch->get_hintergrund(img, get_pos().z+1 >= welt->get_snowline()));
			}
			else {
				weg0->set_bild(besch->get_hintergrund(img, get_pos().z >= welt->get_snowline()));
			}
			weg0->set_yoff(-gr->get_weg_yoff() );

			weg0->set_after_bild(IMG_LEER);
			weg0->set_flag(ding_t::dirty);
#if MULTI_THREAD>1
			weg0->unlock_mutex();
#endif

			if(  weg_t *weg1 = gr->get_weg_nr(1)  ) {
#if MULTI_THREAD>1
				weg1->lock_mutex();
#endif
				weg1->set_yoff(-gr->get_weg_yoff() );
#if MULTI_THREAD>1
				weg1->unlock_mutex();
#endif
			}
		}
		set_yoff( -gr->get_weg_yoff() );
	}
}


image_id bruecke_t::get_after_bild() const
{
	return besch->get_vordergrund(img, get_pos().z+(img>=bruecke_besch_t::N_Start  &&  img<=bruecke_besch_t::W_Start) >= welt->get_snowline());
}


void bruecke_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "bruecke_t" );

	ding_t::rdwr(file);

	const char *s = NULL;

	if(file->is_saving()) {
		s = besch->get_name();
	}
	file->rdwr_str(s);
	file->rdwr_enum(img);

	if(file->is_loading()) {
		besch = brueckenbauer_t::get_besch(s);
		if(besch==NULL) {
			besch = brueckenbauer_t::get_besch(translator::compatibility_name(s));
		}
		if(besch==NULL) {
			dbg->warning( "bruecke_t::rdwr()", "unknown bridge \"%s\" at (%i,%i) will be replaced with best match!", s, get_pos().x, get_pos().y );
			welt->add_missing_paks( s, karte_t::MISSING_BRIDGE );
		}
		guarded_free(const_cast<char *>(s));
	}
}


// correct speed and maintenance
void bruecke_t::laden_abschliessen()
{
	grund_t *gr = welt->lookup(get_pos());
	if(besch==NULL) {
		weg_t *weg = gr->get_weg_nr(0);
		if(weg) {
			besch = brueckenbauer_t::find_bridge(weg->get_waytype(),weg->get_max_speed(),0);
		}
		if(besch==NULL) {
			dbg->fatal("bruecke_t::rdwr()","Unknown bridge type at (%i,%i)", get_pos().x, get_pos().y );
		}
	}

	spieler_t *sp=get_besitzer();
	// change maintenance
	if(besch->get_waytype()!=powerline_wt) {
		weg_t *weg = gr->get_weg(besch->get_waytype());
		if(weg==NULL) {
			dbg->error("bruecke_t::laden_abschliessen()","Bridge without way at(%s)!", gr->get_pos().get_str() );
			weg = weg_t::alloc( besch->get_waytype() );
			gr->neuen_weg_bauen( weg, 0, welt->get_spieler(1) );
		}
		weg->set_max_speed(besch->get_topspeed());
		// take ownership of way
		spieler_t::add_maintenance( weg->get_besitzer(), -weg->get_besch()->get_wartung(), besch->get_finance_waytype());
		weg->set_besitzer(sp);
	}
	spieler_t::add_maintenance( sp,  besch->get_wartung(), besch->get_finance_waytype());
}


// correct speed and maintenance
void bruecke_t::entferne( spieler_t *sp2 )
{
	spieler_t *sp = get_besitzer();
	// change maintenance, reset max-speed and y-offset
	const grund_t *gr = welt->lookup(get_pos());
	if(gr) {
		weg_t *weg = gr->get_weg( besch->get_waytype() );
		if(weg) {
			weg->set_max_speed( weg->get_besch()->get_topspeed() );
			spieler_t::add_maintenance( sp,  weg->get_besch()->get_wartung(), weg->get_besch()->get_finance_waytype());
			// reset offsets
			weg->set_yoff(0);
			if (gr->get_weg_nr(1)) {
				gr->get_weg_nr(1)->set_yoff(0);
			}
		}
	}
	spieler_t::add_maintenance( sp,  -besch->get_wartung(), besch->get_finance_waytype() );
	spieler_t::book_construction_costs( sp2, -besch->get_preis(), get_pos().get_2d(), besch->get_waytype() );
}


// rotated segment names
static bruecke_besch_t::img_t rotate90_img[12]= {
	bruecke_besch_t::OW_Segment, bruecke_besch_t::NS_Segment,
	bruecke_besch_t::O_Start, bruecke_besch_t::W_Start, bruecke_besch_t::S_Start, bruecke_besch_t::N_Start,
	bruecke_besch_t::O_Rampe, bruecke_besch_t::W_Rampe, bruecke_besch_t::S_Rampe, bruecke_besch_t::N_Rampe,
	bruecke_besch_t::OW_Pillar, bruecke_besch_t::NS_Pillar
};


void bruecke_t::rotate90()
{
	set_yoff(0);
	ding_t::rotate90();
	// the rotated image parameter is just one in front/back
	img = rotate90_img[img];
}


// returns NULL, if removal is allowed
// players can remove public owned ways
const char *bruecke_t::ist_entfernbar(const spieler_t *sp)
{
	if (get_player_nr()==1) {
		return NULL;
	}
	else {
		return ding_t::ist_entfernbar(sp);
	}
}
