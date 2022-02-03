/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../world/simworld.h"
#include "simview.h"
#include "simgraph.h"
#include "viewport.h"

#include "../simticker.h"
#include "../simdebug.h"
#include "../obj/simobj.h"
#include "../simconst.h"
#include "../world/simplan.h"
#include "../tool/simmenu.h"
#include "../player/simplay.h"
#include "../descriptor/ground_desc.h"
#include "../ground/wasser.h"
#include "../dataobj/environment.h"
#include "../obj/zeiger.h"
#include "../utils/simrandom.h"

uint16 win_get_statusbar_height(); // simwin.h

main_view_t::main_view_t(karte_t *welt)
{
	this->welt = welt;
	outside_visible = true;
	viewport = welt->get_viewport();
	assert(welt  &&  viewport);
}

#if COLOUR_DEPTH != 0
static const sint8 hours2night[] =
{
	4,4,4,4,4,4,4,4,
	4,4,4,4,3,2,1,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,1,
	2,3,4,4,4,4,4,4
};
#endif

#ifdef MULTI_THREAD
#include "../utils/simthread.h"

bool spawned_threads=false; // global job indicator array
static simthread_barrier_t display_barrier_start;
static simthread_barrier_t display_barrier_end;

// to start a thread
typedef struct{
	main_view_t *show_routine;
	koord   lt_cl, wh_cl; // pos/size of clipping rect for this thread
	koord   lt, wh;       // pos/size of region to display. set larger than clipping for correct display of trees at thread seams
	sint16  y_min;
	sint16  y_max;
	sint8   thread_num;
} display_region_param_t;

// now the parameters
static display_region_param_t ka[MAX_THREADS];

void *display_region_thread( void *ptr )
{
	display_region_param_t *view = reinterpret_cast<display_region_param_t *>(ptr);

	while(true) {
		simthread_barrier_wait( &display_barrier_start ); // wait for all to start
		clear_all_poly_clip( view->thread_num );
		display_set_clip_wh( view->lt_cl.x, view->lt_cl.y, view->wh_cl.x, view->wh_cl.y, view->thread_num );
		view->show_routine->display_region( view->lt, view->wh, view->y_min, view->y_max, false, true, view->thread_num );
		simthread_barrier_wait( &display_barrier_end ); // wait for all to finish
	}
}

/* The following mutex is only needed for smart cursor */
// mutex for changing settings on hiding buildings/trees
static pthread_mutex_t hide_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool threads_req_pause = false;  // set true to pause all threads to display smartcursor region single threaded
static uint8 num_threads_paused = 0; // number of threads in the paused state
static pthread_cond_t hiding_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t waiting_cond = PTHREAD_COND_INITIALIZER;

#if COLOUR_DEPTH != 0
static bool can_multithreading = true;
#endif
#endif


void main_view_t::display(bool force_dirty)
{
	const uint32 rs = get_random_seed();

#if COLOUR_DEPTH != 0
	DBG_DEBUG4("main_view_t::display", "starting ...");
	display_set_image_proc(true);

	const sint16 disp_width = display_get_width();
	const sint16 disp_real_height = display_get_height();
	const sint16 IMG_SIZE = get_tile_raster_width();

	const sint16 disp_height = display_get_height() - win_get_statusbar_height() - (!ticker::empty() ? TICKER_HEIGHT : 0);

	scr_rect clip_rr(0, env_t::iconsize.w, disp_width, disp_height - env_t::iconsize.h);
	switch (env_t::menupos) {
	case MENU_TOP:
		// rect default
		break;
	case MENU_BOTTOM:
		clip_rr.y = win_get_statusbar_height() + (!ticker::empty() ? TICKER_HEIGHT : 0);
		break;
	case MENU_LEFT:
		clip_rr = scr_rect(env_t::iconsize.w, 0, disp_width - env_t::iconsize.w, disp_height);
		break;
	case MENU_RIGHT:
		clip_rr = scr_rect(0, 0, disp_width - env_t::iconsize.w, disp_height);
		break;
	}
	display_set_clip_wh(clip_rr.x, clip_rr.y, clip_rr.w, clip_rr.h);

	// redraw everything?
	force_dirty = force_dirty || welt->is_dirty();
	welt->unset_dirty();
	if(  force_dirty  ) {
		mark_screen_dirty();
		welt->set_background_dirty();
		force_dirty = false;
	}

	const int dpy_width = display_get_width()/IMG_SIZE + 2;
	const int dpy_height = (disp_real_height*4)/IMG_SIZE;

	const int i_off = viewport->get_world_position().x + viewport->get_viewport_ij_offset().x;
	const int j_off = viewport->get_world_position().y + viewport->get_viewport_ij_offset().y;
	const int const_x_off = viewport->get_x_off();
	const int const_y_off = viewport->get_y_off();

	// change to night mode?
	// images will be recalculated only, when there has been a change, so we set always
	if(grund_t::underground_mode == grund_t::ugm_all) {
		display_day_night_shift(0);
	}
	else if(!env_t::night_shift) {
		display_day_night_shift(env_t::daynight_level);
	}
	else {
		// calculate also days if desired
		uint32 month = welt->get_last_month();
		const uint32 ticks_this_month = welt->get_ticks() % welt->ticks_per_world_month;
		uint32 hours2;
		if (env_t::show_month > env_t::DATE_FMT_MONTH) {
			static sint32 days_per_month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
			hours2 = (((sint64)ticks_this_month*days_per_month[month]) >> (welt->ticks_per_world_month_shift-17));
			hours2 = ((hours2*3) / 8192) % 48;
		}
		else {
			hours2 = ( (ticks_this_month * 3) >> (welt->ticks_per_world_month_shift-4) )%48;
		}
		display_day_night_shift(hours2night[hours2]+env_t::daynight_level);
	}

	// not very elegant, but works:
	// fill everything with black for Underground mode ...
	if( grund_t::underground_mode ) {
		display_fillbox_wh_rgb(clip_rr.x, clip_rr.y, clip_rr.w, clip_rr.h, color_idx_to_rgb(COL_BLACK), force_dirty);
	}
	else if( welt->is_background_dirty()  &&  outside_visible  ) {
		// we check if background will be visible, no need to clear screen if it's not.
		display_background(clip_rr.x, clip_rr.y, clip_rr.w, clip_rr.h, force_dirty);
		welt->unset_background_dirty();
		// reset
		outside_visible = false;
	}
	// to save calls to grund_t::get_disp_height
	// gr->get_disp_height() == min(gr->get_hoehe(), hmax_ground)
	const sint8 hmax_ground = (grund_t::underground_mode==grund_t::ugm_level) ? grund_t::underground_level : 127;

	// lower limit for y: display correctly water/outside graphics at upper border of screen
	int y_min = (-const_y_off + 4*tile_raster_scale_y( min(hmax_ground, welt->min_height)*TILE_HEIGHT_STEP, IMG_SIZE )
					+ 4*(clip_rr.y-IMG_SIZE)-IMG_SIZE/2-1) / IMG_SIZE;

	// prepare view
	rect_t const world_rect(koord(0, 0), welt->get_size());

	koord const estimated_min(((y_min+(-2-((y_min+dpy_width) & 1))) >> 1) + i_off,
		((y_min-(clip_rr.w - const_x_off) / (IMG_SIZE/2) - 1) >> 1) + j_off);

	sint16 const worst_case_mountain_extra = (welt->max_height - welt->min_height) / 2;
	koord const estimated_max((((dpy_height+4*4)+(disp_width - const_x_off) / (IMG_SIZE/2) - 1) >> 1) + i_off + worst_case_mountain_extra,
		(((dpy_height+4*4)-(-2-(((dpy_height+4*4)+dpy_width) & 1))) >> 1) + j_off + worst_case_mountain_extra);

	rect_t view_rect(estimated_min, estimated_max - estimated_min + koord(1, 1));
	view_rect.mask(world_rect);

	if (view_rect != viewport->prepared_rect) {
		welt->prepare_tiles(view_rect, viewport->prepared_rect);
		viewport->prepared_rect = view_rect;
	}

#ifdef MULTI_THREAD
	if(  can_multithreading  ) {
		if(  !spawned_threads  ) {
			// we can do the parallel display using posix threads ...
			pthread_t thread[MAX_THREADS];
			/* Initialize and set thread detached attribute */
			pthread_attr_t attr;
			pthread_attr_init( &attr );
			pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
			// init barrier
			simthread_barrier_init( &display_barrier_start, NULL, env_t::num_threads );
			simthread_barrier_init( &display_barrier_end, NULL, env_t::num_threads );

			for(  int t = 0;  t < env_t::num_threads - 1;  t++  ) {
				if(  pthread_create( &thread[t], &attr, display_region_thread, (void *)&ka[t] )  ) {
					can_multithreading = false;
					dbg->error( "main_view_t::display()", "cannot multi-thread, error at thread #%i", t+1 );
					return;
				}
			}
			spawned_threads = true;
			pthread_attr_destroy( &attr );
		}

		// set parameter for each thread
		const scr_coord_val wh_x = clip_rr.w / env_t::num_threads;
		scr_coord_val lt_x = clip_rr.x;
		for(  int t = 0;  t < env_t::num_threads - 1;  t++  ) {
			ka[t].show_routine = this;
			ka[t].lt_cl = koord( lt_x, clip_rr.y );
			ka[t].wh_cl = koord( wh_x, clip_rr.h );
			ka[t].lt = ka[t].lt_cl - koord( IMG_SIZE/2, 0 ); // process tiles IMG_SIZE/2 outside clipping range for correct tree display at thread seams
			ka[t].wh = ka[t].wh_cl + koord( IMG_SIZE, 0 );
			ka[t].y_min = y_min;
			ka[t].y_max = dpy_height + 4 * 4;
			ka[t].thread_num = t;
			lt_x += wh_x;
		}

		// init variables required to draw smart cursor
		threads_req_pause = false;
		num_threads_paused = 0;

		// and start drawing
		simthread_barrier_wait( &display_barrier_start );

		// the last we can run ourselves, setting clip_wh to the screen edge instead of wh_x (in case disp_width % num_threads != 0)
		clear_all_poly_clip( env_t::num_threads - 1 );
		display_set_clip_wh( lt_x, clip_rr.y, clip_rr.w, clip_rr.h, env_t::num_threads - 1 );
		display_region( koord( lt_x - IMG_SIZE / 2, clip_rr.y ), koord( clip_rr.x + clip_rr.w + IMG_SIZE, clip_rr.h ), y_min, dpy_height + 4 * 4, false, true, env_t::num_threads - 1 );

		simthread_barrier_wait( &display_barrier_end );

		clear_all_poly_clip( 0 );
		display_set_clip_wh(clip_rr.x, clip_rr.y, clip_rr.w, clip_rr.h);
	}
	else {
		// slow serial way of display
		clear_all_poly_clip( 0 );
		display_region( koord(clip_rr.x, clip_rr.y), koord(clip_rr.w, clip_rr.h), y_min, dpy_height + 4 * 4, false, false, 0 );
	}
#else
	clear_all_poly_clip();
	display_region(koord(clip_rr.x, clip_rr.y), koord(clip_rr.w, clip_rr.h), y_min, dpy_height + 4 * 4, false );
#endif

	// and finally overlays (station coverage and signs)
	bool plotted = false; // display overlays even on very large mountains
	for(sint16 y=y_min; y<dpy_height+4*4  ||  plotted; y++) {
		const sint16 ypos = y*(IMG_SIZE/4) + const_y_off;
		plotted = false;

		for( sint16 x = -2-((y+dpy_width) & 1); (x*(IMG_SIZE/2) + const_x_off)<clip_rr.x+clip_rr.w; x += 2 ) {

			const sint16 i = ((y + x) >> 1) + i_off;
			const sint16 j = ((y - x) >> 1) + j_off;
			const sint16 xpos = x * (IMG_SIZE / 2) + const_x_off;

			if(  xpos+IMG_SIZE>0  ) {
				const planquadrat_t *plan=welt->access(i,j);
				if(plan  &&  plan->get_kartenboden()) {
					const grund_t *gr = plan->get_kartenboden();
					sint16 yypos = ypos - tile_raster_scale_y( min(gr->get_hoehe(),hmax_ground)*TILE_HEIGHT_STEP, IMG_SIZE);
					if(  yypos-IMG_SIZE < clip_rr.get_bottom()  &&  yypos+IMG_SIZE>=clip_rr.y  ) {
						plan->display_overlay( xpos, yypos );
						plotted = true;
					}
				}
			}
		}
	}

	obj_t *zeiger = welt->get_zeiger();
	DBG_DEBUG4("main_view_t::display", "display pointer");
	if( zeiger  &&  zeiger->get_pos() != koord3d::invalid ) {
		bool dirty = zeiger->get_flag(obj_t::dirty);

		scr_coord background_pos = viewport->get_screen_coord(zeiger->get_pos());
		scr_coord pointer_pos = background_pos + viewport->scale_offset(koord(zeiger->get_xoff(),zeiger->get_yoff()));

		// mark the cursor position for all tools (except lower/raise)
		if(zeiger->get_yoff()==Z_PLAN) {
			grund_t *gr = welt->lookup( zeiger->get_pos() );
			if(gr && gr->is_visible()) {
				const FLAGGED_PIXVAL transparent = TRANSPARENT25_FLAG|OUTLINE_FLAG| env_t::cursor_overlay_color;
				if(  gr->get_image()==IMG_EMPTY  ) {
					if(  gr->hat_wege()  ) {
						display_img_blend( gr->obj_bei(0)->get_image(), background_pos.x, background_pos.y, transparent, 0, dirty );
					}
					else {
						display_img_blend( ground_desc_t::get_ground_tile(gr), background_pos.x, background_pos.y, transparent, 0, dirty );
					}
				}
				else if(  gr->get_typ()==grund_t::wasser  ) {
					display_img_blend( ground_desc_t::sea->get_image(gr->get_image(),wasser_t::stage), background_pos.x, background_pos.y, transparent, 0, dirty );
				}
				else {
					display_img_blend( gr->get_image(), background_pos.x, background_pos.y, transparent, 0, dirty );
				}
			}
		}
		zeiger->display( pointer_pos.x , pointer_pos.y  CLIP_NUM_DEFAULT);
		zeiger->clear_flag( obj_t::dirty );
	}

	if(welt) {
		// show players income/cost messages
		switch (env_t::show_money_message) {

			case 0:
				// show messages of all players
				for(int x=0; x<MAX_PLAYER_COUNT; x++) {
					if(  welt->get_player(x)  ) {
						welt->get_player(x)->display_messages();
					}
				}
				break;

			case 1:
				// show message of active player
				int x = welt->get_active_player_nr();
				if(  welt->get_player(x)  ) {
					welt->get_player(x)->display_messages();
				}
				break;
		}

	}

	assert( rs == get_random_seed() ); (void)rs;

#else
	(void)force_dirty;
	(void)rs;
#endif
}

void main_view_t::clear_prepared() const
{
	viewport->prepared_rect.discard_area();
}


#ifdef MULTI_THREAD
void main_view_t::display_region( koord lt, koord wh, sint16 y_min, sint16 y_max, bool /*force_dirty*/, bool threaded, const sint8 clip_num )
#else
void main_view_t::display_region( koord lt, koord wh, sint16 y_min, sint16 y_max, bool /*force_dirty*/ )
#endif
{
	const sint16 IMG_SIZE = get_tile_raster_width();

	const int i_off = viewport->get_world_position().x + viewport->get_viewport_ij_offset().x;
	const int j_off = viewport->get_world_position().y + viewport->get_viewport_ij_offset().y;
	const int const_x_off = viewport->get_x_off();
	const int const_y_off = viewport->get_y_off();

	const int dpy_width = display_get_width() / IMG_SIZE + 2;

	// to save calls to grund_t::get_disp_height
	const sint8 hmax_ground = (grund_t::underground_mode == grund_t::ugm_level) ? grund_t::underground_level : 127;

	// prepare for selectively display
	const koord cursor_pos = welt->get_zeiger() ? welt->get_zeiger()->get_pos().get_2d() : koord(-1000, -1000);
	const bool needs_hiding = !env_t::hide_trees  ||  (env_t::hide_buildings != env_t::ALL_HIDDEN_BUILDING);

	for(  int y = y_min;  y < y_max;  y++  ) {
		const sint16 ypos = y * (IMG_SIZE / 4) + const_y_off;
		// plotted = we plotted something
		bool plotted = false;

		for(  sint16 x = -2 - ((y  +dpy_width) & 1);  (x * (IMG_SIZE / 2) + const_x_off) < (lt.x + wh.x);  x += 2  ) {
			const sint16 i = ((y + x) >> 1) + i_off;
			const sint16 j = ((y - x) >> 1) + j_off;
			const sint16 xpos = x * (IMG_SIZE / 2) + const_x_off;

			if(  xpos + IMG_SIZE > lt.x  ) {
				const koord pos(i, j);
				if(  grund_t* const kb = welt->lookup_kartenboden(pos)  ) {
					const sint16 yypos = ypos - tile_raster_scale_y( min( kb->get_hoehe(), hmax_ground ) * TILE_HEIGHT_STEP, IMG_SIZE );
					if(  yypos - IMG_SIZE < lt.y + wh.y  &&  yypos + IMG_SIZE > lt.y  ) {
#ifdef MULTI_THREAD
						bool force_show_grid = false;
						if(  env_t::hide_under_cursor  ) {
							const uint32 cursor_dist = shortest_distance( pos, cursor_pos );
							if(  cursor_dist <= env_t::cursor_hide_range + 2u  ) {  // +2 to allow for rapid diagonal movement
								kb->set_flag( grund_t::dirty );
								if(  cursor_dist <= env_t::cursor_hide_range  ) {
									force_show_grid = true;
								}
							}
						}
						kb->display_if_visible( xpos, yypos, IMG_SIZE, clip_num, force_show_grid );
#else
						if(  env_t::hide_under_cursor  ) {
							const bool saved_grid = grund_t::show_grid;
							const uint32 cursor_dist = shortest_distance( pos, cursor_pos );
							if(  cursor_dist <= env_t::cursor_hide_range + 2u  ) {
								kb->set_flag( grund_t::dirty );
								if(  cursor_dist <= env_t::cursor_hide_range  ) {
									grund_t::show_grid = true;
								}
							}
							kb->display_if_visible( xpos, yypos, IMG_SIZE );
							grund_t::show_grid = saved_grid;
						}
						else {
							kb->display_if_visible( xpos, yypos, IMG_SIZE );
						}
#endif
						plotted = true;

					}
					// not on screen? We still might need to plot the border ...
					else if(  env_t::draw_earth_border  &&  (pos.x-welt->get_size().x+1 == 0  ||  pos.y-welt->get_size().y+1 == 0)  ) {
						kb->display_border( xpos, yypos, IMG_SIZE  CLIP_NUM_PAR);
					}
				}
				else {
					// check if outside visible
					outside_visible = true;
					if(  env_t::draw_outside_tile  ) {
						const sint16 yypos = ypos - tile_raster_scale_y( welt->min_height * TILE_HEIGHT_STEP, IMG_SIZE );
						display_normal( ground_desc_t::outside->get_image(0), xpos, yypos, 0, true, false  CLIP_NUM_PAR);
					}
				}
			}
		}
		// increase lower bound if nothing is visible
		if(  !plotted  ) {
			if (y == y_min) {
				y_min++;
			}
		}
		// increase upper bound if something is visible
		else {
			if (y == y_max-1) {
				y_max++;
			}
		}
	}

	// and then things (and other ground)
	// especially necessary for vehicles
	for(  int y = y_min;  y < y_max;  y++  ) {
		const sint16 ypos = y * (IMG_SIZE / 4) + const_y_off;

		for(  sint16 x = -2 - ((y + dpy_width) & 1);  (x * (IMG_SIZE / 2) + const_x_off) < (lt.x + wh.x);  x += 2  ) {
			const int i = ((y + x) >> 1) + i_off;
			const int j = ((y - x) >> 1) + j_off;
			const int xpos = x * (IMG_SIZE / 2) + const_x_off;

			if(  xpos + IMG_SIZE > lt.x  ) {
				const planquadrat_t *plan = welt->access(i,j);
				if(  plan  &&  plan->get_kartenboden()  ) {
					const grund_t *gr = plan->get_kartenboden();
					// minimum height: ground height for overground,
					// for the definition of underground_level see grund_t::set_underground_mode
					const sint8 hmin = min( gr->get_hoehe(), grund_t::underground_level );

					// maximum height: 127 for overground, underground level for sliced, ground height-1 for complete underground view
					const sint8 hmax = grund_t::underground_mode == grund_t::ugm_all ? gr->get_hoehe() - (!gr->ist_tunnel()) : grund_t::underground_level;

					/* long version
					switch(grund_t::underground_mode) {
						case ugm_all:
							hmin = -128;
							hmax = gr->get_hoehe()-(!gr->ist_tunnel());
							underground_level = -128;
							break;
						case ugm_level:
							hmin = min(gr->get_hoehe(), underground_level);
							hmax = underground_level;
							underground_level = level;
							break;
						case ugm_none:
							hmin = gr->get_hoehe();
							hmax = 127;
							underground_level = 127;
					} */
					sint16 yypos = ypos - tile_raster_scale_y( min( gr->get_hoehe(), hmax_ground ) * TILE_HEIGHT_STEP, IMG_SIZE );
					if(  yypos - IMG_SIZE * 3 < wh.y + lt.y  &&  yypos + IMG_SIZE > lt.y  ) {
						const koord pos(i,j);
						if(  env_t::hide_under_cursor  &&  needs_hiding  ) {
							// If the corresponding setting is on, then hide trees and buildings under mouse cursor
#ifdef MULTI_THREAD
							if(  threaded  ) {
								pthread_mutex_lock( &hide_mutex );
								if(  threads_req_pause  ) {
									// another thread is requesting we pause
									num_threads_paused++;
									pthread_cond_broadcast( &waiting_cond ); // signal the requesting thread that another thread has paused

									// wait until no longer requested to pause
									while(  threads_req_pause  ) {
										pthread_cond_wait( &hiding_cond, &hide_mutex );
									}

									num_threads_paused--;
								}
								if(  shortest_distance( pos, cursor_pos ) <= env_t::cursor_hide_range  ) {
									// wait until all threads are paused
									threads_req_pause = true;
									while(  num_threads_paused < env_t::num_threads - 1  ) {
										pthread_cond_wait( &waiting_cond, &hide_mutex );
									}

									// proceed with drawing in the hidden area singlethreaded
									const bool saved_hide_trees = env_t::hide_trees;
									const uint8 saved_hide_buildings = env_t::hide_buildings;
									env_t::hide_trees = true;
									env_t::hide_buildings = env_t::ALL_HIDDEN_BUILDING;

									plan->display_obj( xpos, yypos, IMG_SIZE, true, hmin, hmax, clip_num );

									env_t::hide_trees = saved_hide_trees;
									env_t::hide_buildings = saved_hide_buildings;

									// unpause all threads
									threads_req_pause = false;
									pthread_cond_broadcast( &hiding_cond );
									pthread_mutex_unlock( &hide_mutex );
								}
								else {
									// not in the hidden area, draw multithreaded
									pthread_mutex_unlock( &hide_mutex );
									plan->display_obj( xpos, yypos, IMG_SIZE, true, hmin, hmax, clip_num );
								}
							}
							else {
#endif
								if(  shortest_distance( pos, cursor_pos ) <= env_t::cursor_hide_range  ) {
									const bool saved_hide_trees = env_t::hide_trees;
									const uint8 saved_hide_buildings = env_t::hide_buildings;
									env_t::hide_trees = true;
									env_t::hide_buildings = env_t::ALL_HIDDEN_BUILDING;

									plan->display_obj( xpos, yypos, IMG_SIZE, true, hmin, hmax  CLIP_NUM_PAR);

									env_t::hide_trees = saved_hide_trees;
									env_t::hide_buildings = saved_hide_buildings;
								}
								else {
									plan->display_obj( xpos, yypos, IMG_SIZE, true, hmin, hmax  CLIP_NUM_PAR);
								}
#ifdef MULTI_THREAD
							}
#endif
						}
						else {
							// hiding turned off, draw multithreaded
							plan->display_obj( xpos, yypos, IMG_SIZE, true, hmin, hmax  CLIP_NUM_PAR);
						}
					}
				}
			}
		}
	}
#ifdef MULTI_THREAD
	// show thread as paused when finished
	if(  threaded  ) {
		pthread_mutex_lock( &hide_mutex );
		num_threads_paused++;
		pthread_cond_broadcast( &waiting_cond );
		pthread_mutex_unlock( &hide_mutex );
	}
#endif
}


void main_view_t::display_background( scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, bool dirty )
{
	if(  !(env_t::draw_earth_border  &&  env_t::draw_outside_tile)  ) {
		display_fillbox_wh_rgb(xp, yp, w, h, env_t::background_color, dirty );
	}
}
