/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
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
#include "../obj/wayobj.h"

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t bridge_calc_image_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif


bruecke_t::bruecke_t(loadsave_t* const file) :
#ifdef INLINE_OBJ_TYPE
	obj_no_info_t(obj_t::bruecke)
#else
	obj_no_info_t()
#endif
{
	rdwr(file);
}


bruecke_t::bruecke_t(koord3d pos, player_t *player, const bridge_desc_t *desc, bridge_desc_t::img_t img) :
#ifdef INLINE_OBJ_TYPE
	obj_no_info_t(obj_t::bruecke, pos)
#else
	obj_no_info_t(pos)
#endif
{
	this->desc = desc;
	this->img = img;
	set_owner( player );
	player_t::book_construction_costs( get_owner(), -desc->get_value(), get_pos().get_2d(), desc->get_waytype());
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
			lock_mutex();
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

			weg0->set_after_image(IMG_EMPTY);
			if(desc->get_has_own_way_graphics())
			{
				weg0->set_image(IMG_EMPTY);
				weg0->set_yoff( -gr->get_weg_yoff() );
				weg0->set_xoff( 0 );

				weg0->set_flag(obj_t::dirty);
				set_image(display_image);
				set_flag(obj_t::dirty);
			}
			else
			{
				if(slope)
				{
					weg0->set_yoff( -gr->get_weg_yoff() );
					weg0->set_xoff( 0 );
					weg0->calc_image();
					weg0->set_flag(obj_t::dirty);
				}
				set_image(display_image);
				set_flag(obj_t::dirty);
			}

#ifdef MULTI_THREAD
			unlock_mutex();
			weg0->unlock_mutex();
#endif
			if(weg_t *weg1 = gr->get_weg_nr(1) ) {
#ifdef MULTI_THREAD
				weg1->lock_mutex();
#endif
				if(slope)
				{
					weg1->set_yoff( -gr->get_weg_yoff() );
					weg1->set_xoff( 0 );
				}
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
		desc = bridge_builder_t::get_desc(s);
		if(desc==NULL) {
			desc = bridge_builder_t::get_desc(translator::compatibility_name(s));
		}
		if(desc==NULL) {
			dbg->warning( "bruecke_t::rdwr()", "unknown bridge \"%s\" at (%i,%i) will be replaced with best match!", s, get_pos().x, get_pos().y );
			welt->add_missing_paks( s, karte_t::MISSING_BRIDGE );
		}
		guarded_free(const_cast<char *>(s));

		if(  file->get_version() < 112007  &&  env_t::pak_height_conversion_factor==2  ) {
			switch(img) {
				case bridge_desc_t::OW_Segment: img = bridge_desc_t::OW_Segment2; break;
				case bridge_desc_t::NS_Segment: img = bridge_desc_t::NS_Segment2; break;
				case bridge_desc_t::O_Start:    img = bridge_desc_t::O_Start2;    break;
				case bridge_desc_t::W_Start:    img = bridge_desc_t::W_Start2;    break;
				case bridge_desc_t::S_Start:    img = bridge_desc_t::S_Start2;    break;
				case bridge_desc_t::N_Start:    img = bridge_desc_t::N_Start2;    break;
				case bridge_desc_t::O_Ramp:    img = bridge_desc_t::O_Ramp2;    break;
				case bridge_desc_t::W_Ramp:    img = bridge_desc_t::W_Ramp2;    break;
				case bridge_desc_t::S_Ramp:    img = bridge_desc_t::S_Ramp2;    break;
				case bridge_desc_t::N_Ramp:    img = bridge_desc_t::N_Ramp2;    break;
				case bridge_desc_t::OW_Pillar:  img = bridge_desc_t::OW_Pillar2;  break;
				case bridge_desc_t::NS_Pillar:  img = bridge_desc_t::NS_Pillar2;  break;
				default: break;
			}
		}
	}
}


// correct speed, maintenance and weight limits
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
				dbg->fatal("bruecke_t::finish_rd()","Unknown bridge for type %x at (%i,%i)", weg->get_waytype(), get_pos().x, get_pos().y );
			}
		}
		else {
			// assume this is a powerbridge, since otherwise there should be a way
			desc = bridge_builder_t::find_bridge( powerline_wt, 0, welt->get_timeline_year_month() );
			if(desc==NULL) {
				desc = bridge_builder_t::find_bridge( powerline_wt, 0, 0 );
			}
			if(desc==NULL) {
				dbg->fatal("bruecke_t::finish_rd()","No powerbridge to built bridge type at (%i,%i)", get_pos().x, get_pos().y );
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
			gr->neuen_weg_bauen( weg, 0, welt->get_public_player());
		}

		const way_desc_t* way_desc = weg->get_desc();

		const slope_t::type hang = gr->get_weg_hang();
		if(hang != slope_t::flat)
		{
			const uint slope_height = (hang & 7) ? 1 : 2;
			if(slope_height == 1)
			{
				weg->set_max_speed(min(desc->get_topspeed_gradient_1(), way_desc->get_topspeed_gradient_1()));
			}
			else
			{
				weg->set_max_speed(min(desc->get_topspeed_gradient_2(), way_desc->get_topspeed_gradient_2()));
			}
		}
		else
		{
			weg->set_max_speed(min(desc->get_topspeed(), way_desc->get_topspeed()));
		}

		weg->set_bridge_weight_limit(desc->get_max_weight());

		// take ownership of way
		player_t::add_maintenance( weg->get_owner(), -weg->get_desc()->get_maintenance(), desc->get_finance_waytype());
		weg->set_owner(player);
	}
	player_t::add_maintenance( player,  desc->get_maintenance(), desc->get_finance_waytype());

	// with double heights may need to correct image on load (not all desc have double images)
	// at present only start images have 2 height variants, others to follow...
	if(  !gr->ist_karten_boden()) {
		if(  desc->get_waytype() != powerline_wt  ) {
			//img = desc->get_straight( gr->get_weg_ribi_unmasked( desc->get_waytype() ) );
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
			const way_desc_t* const way_desc = weg0->get_desc();
			const slope_t::type hang = gr ? gr->get_weg_hang() : slope_t::flat;
			if(hang != slope_t::flat)
				{
					const uint slope_height = (hang & 7) ? 1 : 2;
					if(slope_height == 1)
					{
						weg0->set_max_speed(min(desc->get_topspeed_gradient_1(), way_desc->get_topspeed_gradient_1()));
					}
					else
					{
						weg0->set_max_speed(min(desc->get_topspeed_gradient_2(), way_desc->get_topspeed_gradient_2()));
					}
				}
				else
				{
					weg0->set_max_speed(min(desc->get_topspeed(), way_desc->get_topspeed()));
				}
			player_t::add_maintenance( player,  way_desc->get_maintenance(), way_desc->get_finance_waytype());
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
	player_t::book_construction_costs( player2, -desc->get_value(), get_pos().get_2d(), desc->get_waytype() );
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
const char *bruecke_t:: is_deletable(const player_t *player)
{
	if(  get_player_nr()==welt->get_public_player()->get_player_nr()  ) {
		return NULL;
	}
	return obj_t::is_deletable(player);
}

#ifdef MULTI_THREAD
void bruecke_t::lock_mutex()
{
	pthread_mutex_lock( &bridge_calc_image_mutex );
}


void bruecke_t::unlock_mutex()
{
	pthread_mutex_unlock( &bridge_calc_image_mutex );
}
#endif
