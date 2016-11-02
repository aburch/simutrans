/*
 * Brueckenteile (sichtbar)
 *
 * Hj. Malthaner
 */

#include "../simworld.h"
#include "../simtypes.h"
#include "../simobj.h"
#include "../boden/grund.h"
#include "../player/simplay.h"
#include "../display/simimg.h"
#include "../simmem.h"
#include "../bauer/brueckenbauer.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "bruecke.h"



bruecke_t::bruecke_t(loadsave_t* const file) : obj_no_info_t()
{
	rdwr(file);
}


bruecke_t::bruecke_t(koord3d pos, player_t *player, const bruecke_besch_t *besch, bruecke_besch_t::img_t img) :
 obj_no_info_t(pos)
{
	this->besch = besch;
	this->img = img;
	set_owner( player );
	player_t::book_construction_costs( get_owner(), -besch->get_preis(), get_pos().get_2d(), besch->get_waytype());
}


// single height segments
static bruecke_besch_t::img_t single_img[24]= {
	bruecke_besch_t::NS_Segment, bruecke_besch_t::OW_Segment,
	bruecke_besch_t::N_Start, bruecke_besch_t::S_Start, bruecke_besch_t::O_Start, bruecke_besch_t::W_Start,
	bruecke_besch_t::N_Rampe, bruecke_besch_t::S_Rampe, bruecke_besch_t::O_Rampe, bruecke_besch_t::W_Rampe,
	bruecke_besch_t::NS_Pillar, bruecke_besch_t::OW_Pillar,
	bruecke_besch_t::NS_Segment, bruecke_besch_t::OW_Segment,
	bruecke_besch_t::N_Start, bruecke_besch_t::S_Start, bruecke_besch_t::O_Start, bruecke_besch_t::W_Start,
	bruecke_besch_t::N_Rampe, bruecke_besch_t::S_Rampe, bruecke_besch_t::O_Rampe, bruecke_besch_t::W_Rampe,
	bruecke_besch_t::NS_Pillar, bruecke_besch_t::OW_Pillar
};

void bruecke_t::calc_image()
{
	grund_t *gr=welt->lookup(get_pos());
	if(gr) {
		// if we are on the bridge, put the image into the ground, so we can have two ways ...
		if(  weg_t *weg0 = gr->get_weg_nr(0)  ) {
#ifdef MULTI_THREAD
			weg0->lock_mutex();
#endif
			// if on a slope then start of bridge - take the upper value
			const hang_t::typ slope = gr->get_grund_hang();
			bool is_snow = welt->get_climate( get_pos().get_2d() ) == arctic_climate  ||  get_pos().z + hang_t::max_diff(slope) >= welt->get_snowline();

			// handle cases where old bridges don't have correct images
			image_id display_image=besch->get_hintergrund( img, is_snow );
			if(  display_image==IMG_LEER && besch->get_vordergrund( img, is_snow )==IMG_LEER  ) {
				display_image=besch->get_hintergrund( single_img[img], is_snow );
			}
			weg0->set_bild( display_image );

			weg0->set_yoff(-gr->get_weg_yoff() );

			weg0->set_after_bild(IMG_LEER);
			weg0->set_flag(obj_t::dirty);
#ifdef MULTI_THREAD
			weg0->unlock_mutex();
#endif

			if(  weg_t *weg1 = gr->get_weg_nr(1)  ) {
#ifdef MULTI_THREAD
				weg1->lock_mutex();
#endif
				weg1->set_yoff(-gr->get_weg_yoff() );
#ifdef MULTI_THREAD
				weg1->unlock_mutex();
#endif
			}
		}
		set_yoff( -gr->get_weg_yoff() );
	}
}


image_id bruecke_t::get_front_image() const
{
	grund_t *gr=welt->lookup(get_pos());
	// if on a slope then start of bridge - take the upper value
	const hang_t::typ slope = gr->get_grund_hang();
	bool is_snow = welt->get_climate( get_pos().get_2d() ) == arctic_climate  ||  get_pos().z + hang_t::max_diff(slope) >= welt->get_snowline();
	// handle cases where old bridges don't have correct images
	image_id display_image=besch->get_vordergrund( img, is_snow );
	if(  display_image==IMG_LEER && besch->get_hintergrund( img, is_snow )==IMG_LEER  ) {
		display_image=besch->get_vordergrund( single_img[img], is_snow );
	}
	return display_image;
}


void bruecke_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "bruecke_t" );

	obj_t::rdwr(file);

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

		if(  file->get_version() < 112007  &&  env_t::pak_height_conversion_factor==2  ) {
			switch(img) {
				case bruecke_besch_t::OW_Segment: img = bruecke_besch_t::OW_Segment2; break;
				case bruecke_besch_t::NS_Segment: img = bruecke_besch_t::NS_Segment2; break;
				case bruecke_besch_t::O_Start:    img = bruecke_besch_t::O_Start2;    break;
				case bruecke_besch_t::W_Start:    img = bruecke_besch_t::W_Start2;    break;
				case bruecke_besch_t::S_Start:    img = bruecke_besch_t::S_Start2;    break;
				case bruecke_besch_t::N_Start:    img = bruecke_besch_t::N_Start2;    break;
				case bruecke_besch_t::O_Rampe:    img = bruecke_besch_t::O_Rampe2;    break;
				case bruecke_besch_t::W_Rampe:    img = bruecke_besch_t::W_Rampe2;    break;
				case bruecke_besch_t::S_Rampe:    img = bruecke_besch_t::S_Rampe2;    break;
				case bruecke_besch_t::N_Rampe:    img = bruecke_besch_t::N_Rampe2;    break;
				case bruecke_besch_t::OW_Pillar:  img = bruecke_besch_t::OW_Pillar2;  break;
				case bruecke_besch_t::NS_Pillar:  img = bruecke_besch_t::NS_Pillar2;  break;
				default: break;
			}
		}
	}
}


// correct speed and maintenance
void bruecke_t::finish_rd()
{
	grund_t *gr = welt->lookup(get_pos());
	if(besch==NULL) {
		if(  weg_t *weg = gr->get_weg_nr(0)  ) {
			besch = brueckenbauer_t::find_bridge( weg->get_waytype(), weg->get_max_speed(), welt->get_timeline_year_month() );
			if(besch==NULL) {
				besch = brueckenbauer_t::find_bridge( weg->get_waytype(), weg->get_max_speed(), 0 );
			}
			if(besch==NULL) {
				dbg->fatal("bruecke_t::finish_rd()","Unknown bridge for type %x at (%i,%i)", weg->get_waytype(), get_pos().x, get_pos().y );
			}
		}
		else {
			// assume this is a powerbridge, since otherwise there should be a way
			besch = brueckenbauer_t::find_bridge( powerline_wt, 0, welt->get_timeline_year_month() );
			if(besch==NULL) {
				besch = brueckenbauer_t::find_bridge( powerline_wt, 0, 0 );
			}
			if(besch==NULL) {
				dbg->fatal("bruecke_t::finish_rd()","No powerbridge to built bridge type at (%i,%i)", get_pos().x, get_pos().y );
			}
		}
	}

	player_t *player=get_owner();
	// change maintenance
	if(besch->get_waytype()!=powerline_wt) {
		weg_t *weg = gr->get_weg(besch->get_waytype());
		if(weg==NULL) {
			dbg->error("bruecke_t::finish_rd()","Bridge without way at(%s)!", gr->get_pos().get_str() );
			weg = weg_t::alloc( besch->get_waytype() );
			gr->neuen_weg_bauen( weg, 0, welt->get_public_player() );
		}
		weg->set_max_speed(besch->get_topspeed());
		// take ownership of way
		player_t::add_maintenance( weg->get_owner(), -weg->get_besch()->get_wartung(), besch->get_finance_waytype());
		weg->set_owner(player);
	}
	player_t::add_maintenance( player,  besch->get_wartung(), besch->get_finance_waytype());

	// with double heights may need to correct image on load (not all besch have double images)
	// at present only start images have 2 height variants, others to follow...
	if(  !gr->is_kartenboden  ) {
		if(  besch->get_waytype() != powerline_wt  ) {
			//img = besch->get_simple( gr->get_weg_ribi_unmasked( besch->get_waytype() ) );
		}
	}
	else {
		if(  gr->get_grund_hang() == hang_t::flach  ) {
			//img = besch->get_rampe( gr->get_weg_hang() );
		}
		else {
			img = besch->get_start( gr->get_grund_hang() );
		}
	}
}


// correct speed and maintenance
void bruecke_t::cleanup( player_t *player2 )
{
	player_t *player = get_owner();
	// change maintenance, reset max-speed and y-offset
	const grund_t *gr = welt->lookup(get_pos());
	if(gr) {
		weg_t *weg = gr->get_weg( besch->get_waytype() );
		if(weg) {
			weg->set_max_speed( weg->get_besch()->get_topspeed() );
			player_t::add_maintenance( player,  weg->get_besch()->get_wartung(), weg->get_besch()->get_finance_waytype());
			// reset offsets
			weg->set_yoff(0);
			if (gr->get_weg_nr(1)) {
				gr->get_weg_nr(1)->set_yoff(0);
			}
		}
	}
	player_t::add_maintenance( player,  -besch->get_wartung(), besch->get_finance_waytype() );
	player_t::book_construction_costs( player2, -besch->get_preis(), get_pos().get_2d(), besch->get_waytype() );
}


// rotated segment names
static bruecke_besch_t::img_t rotate90_img[24]= {
	bruecke_besch_t::OW_Segment, bruecke_besch_t::NS_Segment,
	bruecke_besch_t::O_Start, bruecke_besch_t::W_Start, bruecke_besch_t::S_Start, bruecke_besch_t::N_Start,
	bruecke_besch_t::O_Rampe, bruecke_besch_t::W_Rampe, bruecke_besch_t::S_Rampe, bruecke_besch_t::N_Rampe,
	bruecke_besch_t::OW_Pillar, bruecke_besch_t::NS_Pillar,
	bruecke_besch_t::OW_Segment2, bruecke_besch_t::NS_Segment2,
	bruecke_besch_t::O_Start2, bruecke_besch_t::W_Start2, bruecke_besch_t::S_Start2, bruecke_besch_t::N_Start2,
	bruecke_besch_t::O_Rampe2, bruecke_besch_t::W_Rampe2, bruecke_besch_t::S_Rampe2, bruecke_besch_t::N_Rampe2,
	bruecke_besch_t::OW_Pillar2, bruecke_besch_t::NS_Pillar2
};


void bruecke_t::rotate90()
{
	set_yoff(0);
	obj_t::rotate90();
	// the rotated image parameter is just one in front/back
	img = rotate90_img[img];
}


// returns NULL, if removal is allowed
// players can remove public owned ways
const char *bruecke_t::is_deletable(const player_t *player)
{
	if (get_player_nr()==welt->get_public_player()->get_player_nr()) {
		return NULL;
	}
	else {
		return obj_t::is_deletable(player);
	}
}
