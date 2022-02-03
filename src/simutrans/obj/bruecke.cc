/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../world/simworld.h"
#include "../simtypes.h"
#include "simobj.h"
#include "../ground/grund.h"
#include "../player/simplay.h"
#include "../display/simimg.h"
#include "../builder/brueckenbauer.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "bruecke.h"



bruecke_t::bruecke_t(loadsave_t* const file) : obj_no_info_t()
{
	rdwr(file);
}


bruecke_t::bruecke_t(koord3d pos, player_t *player, const bridge_desc_t *desc, bridge_desc_t::img_t img) :
 obj_no_info_t(pos)
{
	this->desc = desc;
	this->img = img;
	set_owner( player );
	player_t::book_construction_costs( get_owner(), -desc->get_price(), get_pos().get_2d(), desc->get_waytype());
}


// single height segments
static bridge_desc_t::img_t single_img[24]= {
	bridge_desc_t::NS_Segment, bridge_desc_t::OW_Segment,
	bridge_desc_t::N_Start, bridge_desc_t::S_Start, bridge_desc_t::O_Start, bridge_desc_t::W_Start,
	bridge_desc_t::N_Ramp, bridge_desc_t::S_Ramp, bridge_desc_t::O_Ramp, bridge_desc_t::W_Ramp,
	bridge_desc_t::NS_Pillar, bridge_desc_t::OW_Pillar,
	bridge_desc_t::NS_Segment, bridge_desc_t::OW_Segment,
	bridge_desc_t::N_Start, bridge_desc_t::S_Start, bridge_desc_t::O_Start, bridge_desc_t::W_Start,
	bridge_desc_t::N_Ramp, bridge_desc_t::S_Ramp, bridge_desc_t::O_Ramp, bridge_desc_t::W_Ramp,
	bridge_desc_t::NS_Pillar, bridge_desc_t::OW_Pillar
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
			const slope_t::type slope = gr->get_grund_hang();
			bool is_snow = welt->get_climate( get_pos().get_2d() ) == arctic_climate  ||  get_pos().z + slope_t::max_diff(slope) >= welt->get_snowline();

			// handle cases where old bridges don't have correct images
			image_id display_image=desc->get_background( img, is_snow );
			if(  display_image==IMG_EMPTY && desc->get_foreground( img, is_snow )==IMG_EMPTY  ) {
				display_image=desc->get_background( single_img[img], is_snow );
			}
			weg0->set_image( display_image );

			// must always set both offsets, because after roation the xoffset contains the yoffset
			weg0->set_yoff( -gr->get_weg_yoff() );
			weg0->set_xoff( 0 );

			weg0->set_foreground_image(IMG_EMPTY);
			weg0->set_flag(obj_t::dirty);
#ifdef MULTI_THREAD
			weg0->unlock_mutex();
#endif

			if(  weg_t *weg1 = gr->get_weg_nr(1)  ) {
#ifdef MULTI_THREAD
				weg1->lock_mutex();
#endif
				weg1->set_yoff( -gr->get_weg_yoff() );
				weg1->set_xoff( 0 );
#ifdef MULTI_THREAD
				weg1->unlock_mutex();
#endif
			}
		}
		set_yoff( -gr->get_weg_yoff() );
		set_xoff( 0 );
	}
}


image_id bruecke_t::get_front_image() const
{
	grund_t *gr=welt->lookup(get_pos());
	// if on a slope then start of bridge - take the upper value
	const slope_t::type slope = gr->get_grund_hang();
	bool is_snow = welt->get_climate( get_pos().get_2d() ) == arctic_climate  ||  get_pos().z + slope_t::max_diff(slope) >= welt->get_snowline();
	// handle cases where old bridges don't have correct images
	image_id display_image=desc->get_foreground( img, is_snow );
	if(  display_image==IMG_EMPTY && desc->get_background( img, is_snow )==IMG_EMPTY  ) {
		display_image=desc->get_foreground( single_img[img], is_snow );
	}
	return display_image;
}


void bruecke_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "bruecke_t" );

	obj_t::rdwr(file);

	const char *s = NULL;

	if(file->is_saving()) {
		s = desc->get_name();
	}
	file->rdwr_str(s);
	file->rdwr_enum(img);

	if(file->is_loading()) {
		if (!s) {
			dbg->fatal("bruecke_t::rdwr", "No bridge name for bridge at (%s)", get_pos().get_str());
		}

		desc = bridge_builder_t::get_desc(s);
		if(desc==NULL) {
			desc = bridge_builder_t::get_desc(translator::compatibility_name(s));
		}
		if(desc==NULL) {
			dbg->warning( "bruecke_t::rdwr", "Unknown bridge \"%s\" at (%s) will be replaced with best match!", s, get_pos().get_str() );
			welt->add_missing_paks( s, karte_t::MISSING_BRIDGE );
		}
		free(const_cast<char *>(s));

		if(  file->is_version_less(112, 7)  &&  env_t::pak_height_conversion_factor==2  ) {
			switch(img) {
				case bridge_desc_t::OW_Segment: img = bridge_desc_t::OW_Segment2; break;
				case bridge_desc_t::NS_Segment: img = bridge_desc_t::NS_Segment2; break;
				case bridge_desc_t::O_Start:    img = bridge_desc_t::O_Start2;    break;
				case bridge_desc_t::W_Start:    img = bridge_desc_t::W_Start2;    break;
				case bridge_desc_t::S_Start:    img = bridge_desc_t::S_Start2;    break;
				case bridge_desc_t::N_Start:    img = bridge_desc_t::N_Start2;    break;
				case bridge_desc_t::O_Ramp:     img = bridge_desc_t::O_Ramp2;     break;
				case bridge_desc_t::W_Ramp:     img = bridge_desc_t::W_Ramp2;     break;
				case bridge_desc_t::S_Ramp:     img = bridge_desc_t::S_Ramp2;     break;
				case bridge_desc_t::N_Ramp:     img = bridge_desc_t::N_Ramp2;     break;
				case bridge_desc_t::OW_Pillar:  img = bridge_desc_t::OW_Pillar2;  break;
				case bridge_desc_t::NS_Pillar:  img = bridge_desc_t::NS_Pillar2;  break;
				default: break;
			}
		}
	}
}


// correct speed and maintenance
void bruecke_t::finish_rd()
{
	grund_t *gr = welt->lookup(get_pos());
	if(desc==NULL) {
		if(  weg_t *weg = gr->get_weg_nr(0)  ) {
			desc = bridge_builder_t::find_bridge( weg->get_waytype(), weg->get_max_speed(), welt->get_timeline_year_month() );
			if(desc==NULL) {
				desc = bridge_builder_t::find_bridge( weg->get_waytype(), weg->get_max_speed(), 0 );
			}
			if(desc==NULL) {
				dbg->fatal("bruecke_t::finish_rd()", "Unknown bridge for type %x at (%s)", weg->get_waytype(), get_pos().get_str() );
			}
		}
		else {
			// assume this is a powerbridge, since otherwise there should be a way
			desc = bridge_builder_t::find_bridge( powerline_wt, 0, welt->get_timeline_year_month() );
			if(desc==NULL) {
				desc = bridge_builder_t::find_bridge( powerline_wt, 0, 0 );
			}
			if(desc==NULL) {
				dbg->fatal("bruecke_t::finish_rd()", "No powerline bridge to build bridge type at (%s)", get_pos().get_str() );
			}
		}
	}

	player_t *player=get_owner();
	// change maintenance
	if(desc->get_waytype()!=powerline_wt) {
		weg_t *weg = gr->get_weg(desc->get_waytype());
		if(weg==NULL) {
			dbg->error("bruecke_t::finish_rd()","Bridge without way at(%s)!", gr->get_pos().get_str() );
			weg = weg_t::alloc( desc->get_waytype() );
			gr->neuen_weg_bauen( weg, 0, welt->get_public_player() );
		}
		weg->set_max_speed(desc->get_topspeed());
		// take ownership of way
		player_t::add_maintenance( weg->get_owner(), -weg->get_desc()->get_maintenance(), desc->get_finance_waytype());
		weg->set_owner(player);
	}
	player_t::add_maintenance( player,  desc->get_maintenance(), desc->get_finance_waytype());

	// with double heights may need to correct image on load (not all desc have double images)
	// at present only start images have 2 height variants, others to follow...
	if(  !gr->ist_karten_boden()  ) {
		if(  desc->get_waytype() != powerline_wt  ) {
			//img = desc->get_simple( gr->get_weg_ribi_unmasked( desc->get_waytype() ) );
		}
	}
	else {
		if(  gr->get_grund_hang() == slope_t::flat  ) {
			//img = desc->get_ramp( gr->get_weg_hang() );
		}
		else {
			img = desc->get_start( gr->get_grund_hang() );
		}
	}
}


// correct speed and maintenance
void bruecke_t::cleanup( player_t *player2 )
{
	player_t *player = get_owner();
	// change maintenance, reset max-speed and y-offset
	if(  const grund_t *gr = welt->lookup(get_pos())  ) {
		if(  weg_t *weg0 = gr->get_weg( desc->get_waytype() )  ) {
			weg0->set_max_speed( weg0->get_desc()->get_topspeed() );
			player_t::add_maintenance( player,  weg0->get_desc()->get_maintenance(), weg0->get_desc()->get_finance_waytype());
			// reset offsets
			weg0->set_xoff(0);
			weg0->set_yoff(0);
			if(  weg_t *weg1 = gr->get_weg_nr(1)  ) {
				weg1->set_xoff(0);
				weg1->set_yoff(0);
			}
		}
	}

	player_t::add_maintenance( player,  -desc->get_maintenance(), desc->get_finance_waytype() );
	player_t::book_construction_costs( player2, -desc->get_price(), get_pos().get_2d(), desc->get_waytype() );
}


// rotated segment names
static bridge_desc_t::img_t rotate90_img[24]= {
	bridge_desc_t::OW_Segment, bridge_desc_t::NS_Segment,
	bridge_desc_t::O_Start, bridge_desc_t::W_Start, bridge_desc_t::S_Start, bridge_desc_t::N_Start,
	bridge_desc_t::O_Ramp, bridge_desc_t::W_Ramp, bridge_desc_t::S_Ramp, bridge_desc_t::N_Ramp,
	bridge_desc_t::OW_Pillar, bridge_desc_t::NS_Pillar,
	bridge_desc_t::OW_Segment2, bridge_desc_t::NS_Segment2,
	bridge_desc_t::O_Start2, bridge_desc_t::W_Start2, bridge_desc_t::S_Start2, bridge_desc_t::N_Start2,
	bridge_desc_t::O_Ramp2, bridge_desc_t::W_Ramp2, bridge_desc_t::S_Ramp2, bridge_desc_t::N_Ramp2,
	bridge_desc_t::OW_Pillar2, bridge_desc_t::NS_Pillar2
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
	if (get_owner_nr()==PUBLIC_PLAYER_NR) {
		return NULL;
	}
	else {
		return obj_t::is_deletable(player);
	}
}
