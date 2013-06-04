/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 */

#include <stdio.h>

#include "simworld.h"
#include "simview.h"
#include "simgraph.h"

#include "simticker.h"
#include "simdebug.h"
#include "simdings.h"
#include "simconst.h"
#include "simplan.h"
#include "simmenu.h"
#include "player/simplay.h"
#include "besch/grund_besch.h"
#include "boden/wasser.h"
#include "dataobj/umgebung.h"
#include "dings/zeiger.h"

#include "simtools.h"

karte_ansicht_t::karte_ansicht_t(karte_t *welt)
{
	this->welt = welt;
	outside_visible = true;
}

static const sint8 hours2night[] =
{
    4,4,4,4,4,4,4,4,
    4,4,4,4,3,2,1,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,1,
    2,3,4,4,4,4,4,4
};

#if MULTI_THREAD>1
// enable barriers by this
#define _XOPEN_SOURCE 600
#include <pthread.h>

bool spawned_threads=false; // global job indicator array
static pthread_barrier_t display_barrier_start;
static pthread_barrier_t display_barrier_end;

// to start a thread
typedef struct{
	karte_ansicht_t *show_routine;
	koord	lt;
	koord	wh;
	sint16	y_min;
	sint16	y_max;
} display_region_param_t;

// now the paramters
static display_region_param_t ka[MULTI_THREAD];

void *display_region_thread( void *ptr )
{
	display_region_param_t *view = reinterpret_cast<display_region_param_t *>(ptr);
	while(true) {
		pthread_barrier_wait( &display_barrier_start );	// wait for all to start
		view->show_routine->display_region( view->lt, view->wh, view->y_min, view->y_max, false, true );
		pthread_barrier_wait( &display_barrier_end );	// wait for all to finish
	}
	return ptr;
}

/* The following mutexes are only needed for smart cursor */
// mutex for accessing grund_t::show_grid
static pthread_mutex_t grid_mutex;
// mutex for changing settings on hiding buildings/trees
static pthread_mutex_t hide_mutex;

static bool can_multithreading = true;
#endif



void karte_ansicht_t::display(bool force_dirty)
{
#if COLOUR_DEPTH != 0
	DBG_DEBUG4("karte_ansicht_t::display", "starting ...");
	display_set_image_proc(true);

	uint32 rs = get_random_seed();
	const sint16 disp_width = display_get_width();
	const sint16 disp_real_height = display_get_height();
	const sint16 menu_height = umgebung_t::iconsize.y;
	const sint16 IMG_SIZE = get_tile_raster_width();

	const sint16 disp_height = display_get_height() - 16 - (!ticker::empty() ? 16 : 0);
	display_set_clip_wh( 0, menu_height, disp_width, disp_height-menu_height );

	// redraw everything?
	force_dirty = force_dirty || welt->is_dirty();
	welt->unset_dirty();
	if(  force_dirty  ) {
		mark_screen_dirty();
		welt->set_background_dirty();
		force_dirty = false;
	}

	const int dpy_width = disp_width/IMG_SIZE + 2;
	const int dpy_height = (disp_real_height*4)/IMG_SIZE;

	// these are the values needed to go directly from a tile to the display
	welt->set_view_ij_offset(
		koord( - disp_width/(2*IMG_SIZE) - disp_real_height/IMG_SIZE,
					disp_width/(2*IMG_SIZE) - disp_real_height/IMG_SIZE	)
	);

	const int i_off = welt->get_world_position().x - disp_width/(2*IMG_SIZE) - disp_real_height/IMG_SIZE;
	const int j_off = welt->get_world_position().y + disp_width/(2*IMG_SIZE) - disp_real_height/IMG_SIZE;
	const int const_x_off = welt->get_x_off();
	const int const_y_off = welt->get_y_off();

	// change to night mode?
	// images will be recalculated only, when there has been a change, so we set always
	if(grund_t::underground_mode == grund_t::ugm_all) {
		display_day_night_shift(0);
	}
	else if(!umgebung_t::night_shift) {
		display_day_night_shift(umgebung_t::daynight_level);
	}
	else {
		// calculate also days if desired
		uint32 month = welt->get_last_month();
		const uint32 ticks_this_month = welt->get_zeit_ms() % welt->ticks_per_world_month;
		uint32 stunden2;
		if (umgebung_t::show_month > umgebung_t::DATE_FMT_MONTH) {
			static sint32 tage_per_month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
			stunden2 = (((sint64)ticks_this_month*tage_per_month[month]) >> (welt->ticks_per_world_month_shift-17));
			stunden2 = ((stunden2*3) / 8192) % 48;
		}
		else {
			stunden2 = ( (ticks_this_month * 3) >> (welt->ticks_per_world_month_shift-4) )%48;
		}
		display_day_night_shift(hours2night[stunden2]+umgebung_t::daynight_level);
	}

	// not very elegant, but works:
	// fill everything with black for Underground mode ...
	if( grund_t::underground_mode ) {
		display_fillbox_wh(0, menu_height, disp_width, disp_height-menu_height, COL_BLACK, force_dirty);
	}
	else if( welt->is_background_dirty()  &&  outside_visible  ) {
		// we check if background will be visible, no need to clear screen if it's not.
		display_background(0, menu_height, disp_width, disp_height-menu_height, force_dirty);
		welt->unset_background_dirty();
		// reset
		outside_visible = false;
	}
	// to save calls to grund_t::get_disp_height
	// gr->get_disp_height() == min(gr->get_hoehe(), hmax_ground)
	const sint8 hmax_ground = (grund_t::underground_mode==grund_t::ugm_level) ? grund_t::underground_level : 127;

	// lower limit for y: display correctly water/outside graphics at upper border of screen
	int y_min = (-const_y_off + 4*tile_raster_scale_y( min(hmax_ground, welt->get_grundwasser())*TILE_HEIGHT_STEP, IMG_SIZE )
					+ 4*(menu_height-IMG_SIZE)-IMG_SIZE/2-1) / IMG_SIZE;

#if MULTI_THREAD>1
	if(  umgebung_t::simple_drawing  &&  can_multithreading  ) {

		if(!spawned_threads) {
			// we can do the parallel display using posix threads ...
			pthread_t thread[MULTI_THREAD];
			/* Initialize and set thread detached attribute */
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			// init barrier
			pthread_barrier_init( &display_barrier_start, NULL, MULTI_THREAD );
			pthread_barrier_init( &display_barrier_end, NULL, MULTI_THREAD );
			// init mutexes
			pthread_mutex_init( &grid_mutex, NULL );
			pthread_mutex_init( &hide_mutex, NULL );

			for(  int t=0;  t<MULTI_THREAD-1;  t++  ) {
				if(  pthread_create(&thread[t], &attr, display_region_thread, (void *)&ka[t])  ) {
					can_multithreading = false;
					dbg->error( "karte_ansicht_t::display()", "cannot multithread, error at thread #%i", t+1 );
					return;
				}
			}

			spawned_threads = true;
			pthread_attr_destroy(&attr);
		}

		// set parameter for each thread
		for(  int t=0;  t<MULTI_THREAD-1;  t++  ) {
			// equals to: display_region( koord(t*(disp_width/NUM_THREADS),menu_height), koord(disp_width/NUM_THREADS,disp_height-menu_height), y_min, dpy_height+4*4, force_dirty );
		   	ka[t].show_routine = this;
			ka[t].lt = koord((t*disp_width)/MULTI_THREAD,menu_height);
			ka[t].wh = koord((disp_width/MULTI_THREAD)-((t==MULTI_THREAD-1)?0:IMG_SIZE),disp_height-menu_height);
			ka[t].y_min = y_min;
			ka[t].y_max = dpy_height+4*4;
		}
		// and start drawing
		pthread_barrier_wait( &display_barrier_start );

		// the last we can run ourselves
		display_region( koord( ((MULTI_THREAD-1)*disp_width)/MULTI_THREAD,menu_height), koord(disp_width/MULTI_THREAD,disp_height-menu_height), y_min, dpy_height+4*4, false, true );

		pthread_barrier_wait( &display_barrier_end );

		// and now draw the overlapping region single threaded with clipping
		for(  int t=1;  t<MULTI_THREAD;  t++  ) {
			KOORD_VAL start_x = (t*disp_width)/MULTI_THREAD-IMG_SIZE;
			display_set_clip_wh( start_x, menu_height, IMG_SIZE, disp_height-menu_height );
			if(grund_t::underground_mode) {
				display_fillbox_wh(start_x, menu_height, IMG_SIZE, disp_height-menu_height, COL_BLACK, force_dirty);
			}
			display_region( koord( start_x,menu_height), koord(IMG_SIZE,disp_height-menu_height), y_min, dpy_height+4*4, false, false );
		}
		display_set_clip_wh( 0, menu_height, disp_width, disp_height-menu_height );
	}
	else
#endif
	{
		// slow serial way of display
		display_region( koord(0,menu_height), koord(disp_width,disp_height-menu_height), y_min, dpy_height+4*4, false, false );
	}

	// and finally overlays (station coverage and signs)
	for(sint16 y=y_min; y<dpy_height+4*4; y++) {

		const sint16 ypos = y*(IMG_SIZE/4) + const_y_off;

		for(sint16 x=-2-((y+dpy_width) & 1); (x*(IMG_SIZE/2) + const_x_off)<disp_width; x+=2) {

			const int i = ((y+x) >> 1) + i_off;
			const int j = ((y-x) >> 1) + j_off;
			const int xpos = x*(IMG_SIZE/2) + const_x_off;

			if(  xpos+IMG_SIZE>0  ) {
				const planquadrat_t *plan=welt->lookup(koord(i,j));
				if(plan  &&  plan->get_kartenboden()) {
					const grund_t *gr = plan->get_kartenboden();
					// minimum height: ground height for overground,
					// for the definition of underground_level see grund_t::set_underground_mode
					const sint8 hmin = min(gr->get_hoehe(), grund_t::underground_level);

					// maximum height: 127 for overground, undergroundlevel for sliced, ground height-1 for complete underground view
					const sint8 hmax = grund_t::underground_mode==grund_t::ugm_all ? gr->get_hoehe()-(!gr->ist_tunnel()) : grund_t::underground_level;

					sint16 yypos = ypos - tile_raster_scale_y( min(gr->get_hoehe(),hmax_ground)*TILE_HEIGHT_STEP, IMG_SIZE);
					if(  yypos-IMG_SIZE<disp_real_height  &&  yypos+IMG_SIZE>=menu_height  ) {
						plan->display_overlay( xpos, yypos, hmin, hmax);
					}
				}
			}
		}
	}

	ding_t *zeiger = welt->get_zeiger();
	DBG_DEBUG4("karte_ansicht_t::display", "display pointer");
	if( zeiger  &&  zeiger->get_pos() != koord3d::invalid ) {
		bool dirty = zeiger->get_flag(ding_t::dirty);
		// better not try to twist your brain to follow the retransformation ...
		const koord diff = zeiger->get_pos().get_2d()-welt->get_world_position()-welt->get_view_ij_offset();
		const sint16 x = (diff.x-diff.y)*(IMG_SIZE/2) + const_x_off;
		const sint16 y = (diff.x+diff.y)*(IMG_SIZE/4) - tile_raster_scale_y( zeiger->get_pos().z*TILE_HEIGHT_STEP, IMG_SIZE) + ((display_get_width()/IMG_SIZE)&1)*(IMG_SIZE/4) + const_y_off;
		// mark the cursor position for all tools (except lower/raise)
		if(zeiger->get_yoff()==Z_PLAN) {
			grund_t *gr = welt->lookup( zeiger->get_pos() );
			if(gr && gr->is_visible()) {
				const PLAYER_COLOR_VAL transparent = TRANSPARENT25_FLAG|OUTLINE_FLAG| umgebung_t::cursor_overlay_color;
				if(  gr->get_bild()==IMG_LEER  ) {
					if(  gr->hat_wege()  ) {
						display_img_blend( gr->obj_bei(0)->get_bild(), x, y, transparent, 0, dirty );
					}
					else {
						display_img_blend( grund_besch_t::get_ground_tile(gr), x, y, transparent, 0, dirty );
					}
				}
				else if(  gr->get_typ()==grund_t::wasser  ) {
					display_img_blend( grund_besch_t::sea->get_bild(gr->get_bild(),wasser_t::stage), x, y, transparent, 0, dirty );
				}
				else {
					display_img_blend( gr->get_bild(), x, y, transparent, 0, dirty );
				}
			}
		}
		zeiger->display( x + tile_raster_scale_x( zeiger->get_xoff(), IMG_SIZE), y + tile_raster_scale_y( zeiger->get_yoff(), IMG_SIZE));
		zeiger->clear_flag(ding_t::dirty);
	}

	if(welt) {
		// show players income/cost messages
		for(int x=0; x<MAX_PLAYER_COUNT; x++) {
			if(  welt->get_spieler(x)  ) {
				welt->get_spieler(x)->display_messages();
			}
		}
	}

	assert( rs == get_random_seed() );

#else
	(void)force_dirty;
#endif
}



void karte_ansicht_t::display_region( koord lt, koord wh, sint16 y_min, const sint16 y_max, bool /*force_dirty*/, bool threaded )
{
	const sint16 IMG_SIZE = get_tile_raster_width();

	const int i_off = welt->get_world_position().x - display_get_width()/(2*IMG_SIZE) - display_get_height()/IMG_SIZE;
	const int j_off = welt->get_world_position().y + display_get_width()/(2*IMG_SIZE) - display_get_height()/IMG_SIZE;
	const int const_x_off = welt->get_x_off();
	const int const_y_off = welt->get_y_off();

	const int dpy_width = display_get_width()/IMG_SIZE + 2;

	// to save calls to grund_t::get_disp_height
	const sint8 hmax_ground = (grund_t::underground_mode==grund_t::ugm_level) ? grund_t::underground_level : 127;

	// prepare for selectively display
	const koord cursor_pos = welt->get_zeiger() ? welt->get_zeiger()->get_pos().get_2d() : koord(-1000,-1000);
#if MULTI_THREAD>1
	if(  threaded  ) {
		pthread_mutex_lock( &grid_mutex  );
		pthread_mutex_lock( &hide_mutex  );
	}
#endif
	const bool saved_grid = grund_t::show_grid;
	const bool saved_hide_trees = umgebung_t::hide_trees;
	const uint8 saved_hide_buildings = umgebung_t::hide_buildings;
#if MULTI_THREAD>1
	if(  threaded  ) {
		pthread_mutex_unlock( &grid_mutex  );
		pthread_mutex_unlock( &hide_mutex  );
	}
#endif
	bool lock_restore_grid = false;	// true while showing grid
	bool lock_restore_hiding = false; // true while hiding buildings/trees around cursor
	const bool needs_hiding = !umgebung_t::hide_trees  |  (umgebung_t::hide_buildings != umgebung_t::ALL_HIDDEN_BUIDLING);

	for( int y=y_min;  y<y_max;  y++  ) {

		const sint16 ypos = y*(IMG_SIZE/4) + const_y_off;
		// plotted = we plotted something for y=lower bound
		bool plotted = y>y_min;

		for(  sint16 x=-2-((y+dpy_width) & 1);  (x*(IMG_SIZE/2) + const_x_off) < (lt.x+wh.x);  x+=2  ) {

			const sint16 i = ((y+x) >> 1) + i_off;
			const sint16 j = ((y-x) >> 1) + j_off;
			const sint16 xpos = x*(IMG_SIZE/2) + const_x_off;

			if(  xpos+IMG_SIZE>lt.x  ) {
				const koord pos(i,j);
				if(  grund_t* const kb = welt->lookup_kartenboden(pos)  ) {
					const sint16 yypos = ypos - tile_raster_scale_y(min(kb->get_hoehe(), hmax_ground) * TILE_HEIGHT_STEP, IMG_SIZE);
					if(yypos-IMG_SIZE<lt.y+wh.y  &&  yypos+IMG_SIZE>lt.y) {

						if(  !saved_grid  &&  umgebung_t::hide_under_cursor  ) {
							// If the corresponding setting is on, then hide trees and buildings under mouse cursor
							if(  koord_distance(pos,cursor_pos) < umgebung_t::cursor_hide_range  ) {
								if(  !lock_restore_grid  ) {
									lock_restore_grid = true;
#if MULTI_THREAD>1
									if(  lock_restore_grid  &&  threaded  ) {
										pthread_mutex_lock( &grid_mutex  );
									}
#endif
								}
								grund_t::show_grid = true;
								kb->set_flag(grund_t::dirty);
							}
							else if(  lock_restore_grid  ) {
								kb->set_flag(grund_t::dirty);
								grund_t::show_grid = false;
								lock_restore_grid = false;
#if MULTI_THREAD>1
								if(  threaded  ) {
									pthread_mutex_unlock( &grid_mutex  );
								}
#endif
							}
						}
#if MULTI_THREAD>1
						if(  !lock_restore_grid  &&  grund_t::show_grid != saved_grid  &&  threaded  ) {
							pthread_mutex_lock( &grid_mutex  );
							kb->display_if_visible(xpos, yypos, IMG_SIZE);
							pthread_mutex_unlock( &grid_mutex  );
						}
						else
#endif
						kb->display_if_visible(xpos, yypos, IMG_SIZE);
						plotted = true;
					}
					// not on screen? We still might need to plot the border ...
					else if(  umgebung_t::draw_earth_border  &&  (pos.x-welt->get_size().x+1 == 0  ||  pos.y-welt->get_size().y+1 == 0)  ) {
						kb->display_border( xpos, yypos, IMG_SIZE );
					}
				}
				else {
					// check if ouside visible
					outside_visible = true;
					if(  umgebung_t::draw_outside_tile  ) {
						const sint16 yypos = ypos - tile_raster_scale_y(welt->get_grundwasser()*TILE_HEIGHT_STEP, IMG_SIZE);
						display_normal( grund_besch_t::ausserhalb->get_bild(0), xpos, yypos, 0, true, false );
					}
				}
			}
		}
		// increase lower bound if nothing is visible
		if (!plotted) {
			y_min++;
		}
	}

	if(  lock_restore_grid  ) {
		grund_t::show_grid = saved_grid;
		lock_restore_grid = false;
#if MULTI_THREAD>1
		if(  threaded  ) {
			pthread_mutex_unlock( &grid_mutex  );
		}
#endif
	}

	// and then things (and other ground)
	// especially necessary for vehicles
	for(  int y=y_min;  y<y_max;  y++  ) {

		const sint16 ypos = y*(IMG_SIZE/4) + const_y_off;

		for(  sint16 x=-2-((y+dpy_width) & 1);  (x*(IMG_SIZE/2) + const_x_off)<(lt.x+wh.x);  x+=2  ) {

			const int i = ((y+x) >> 1) + i_off;
			const int j = ((y-x) >> 1) + j_off;
			const int xpos = x*(IMG_SIZE/2) + const_x_off;

			if(  xpos+IMG_SIZE>lt.x  ) {
				const koord pos(i,j);
				const planquadrat_t *plan=welt->lookup(pos);
				if(plan  &&  plan->get_kartenboden()) {
					const grund_t *gr = plan->get_kartenboden();
					// minimum height: ground height for overground,
					// for the definition of underground_level see grund_t::set_underground_mode
					const sint8 hmin = min(gr->get_hoehe(), grund_t::underground_level);

					// maximum height: 127 for overground, undergroundlevel for sliced, ground height-1 for complete underground view
					const sint8 hmax = grund_t::underground_mode==grund_t::ugm_all ? gr->get_hoehe()-(!gr->ist_tunnel()) : grund_t::underground_level;

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
					sint16 yypos = ypos - tile_raster_scale_y( min(gr->get_hoehe(),hmax_ground)*TILE_HEIGHT_STEP, IMG_SIZE);
					if(yypos-IMG_SIZE*3<wh.y+lt.y  &&  yypos+IMG_SIZE>lt.y) {

						if(  umgebung_t::hide_under_cursor  &&  needs_hiding  ) {
							if(  koord_distance(pos,cursor_pos) < umgebung_t::cursor_hide_range  ) {
								// If the corresponding setting is on, then hide trees and buildings under mouse cursor
								if(  !lock_restore_hiding  ) {
									lock_restore_hiding = true;
#if MULTI_THREAD>1
									if(  threaded  ) {
										pthread_mutex_lock( &hide_mutex  );
									}
#endif
								}
								umgebung_t::hide_trees = true;
								umgebung_t::hide_buildings = umgebung_t::ALL_HIDDEN_BUIDLING;
							}
							else if(  lock_restore_hiding  ) {
								lock_restore_hiding = false;
								umgebung_t::hide_trees = saved_hide_trees;
								umgebung_t::hide_buildings = saved_hide_buildings;
#if MULTI_THREAD>1
								if(  threaded  ) {
									pthread_mutex_unlock( &hide_mutex  );
								}
#endif
							}
						}

#if MULTI_THREAD>1
						if(  !lock_restore_hiding  &&  needs_hiding  &&  threaded  &&  (umgebung_t::hide_trees != saved_hide_trees  ||  umgebung_t::hide_buildings != saved_hide_buildings  )  ) {
							pthread_mutex_lock( &hide_mutex  );
							plan->display_dinge(xpos, yypos, IMG_SIZE, true, hmin, hmax);
							pthread_mutex_unlock( &hide_mutex  );
						}
						else
#endif
						plan->display_dinge(xpos, yypos, IMG_SIZE, true, hmin, hmax);
					}
				}
			}
		}
	}
	// relese lock if still set
	if(  lock_restore_hiding  ) {
		lock_restore_hiding = false;
		umgebung_t::hide_trees = saved_hide_trees;
		umgebung_t::hide_buildings = saved_hide_buildings;
#if MULTI_THREAD>1
		if(  threaded  ) {
			pthread_mutex_unlock( &hide_mutex  );
		}
#endif
	}
	(void) threaded;
}


void karte_ansicht_t::display_background( KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, bool dirty )
{
	if(  !(umgebung_t::draw_earth_border  &&  umgebung_t::draw_outside_tile)  ) {
		display_fillbox_wh(xp, yp, w, h, umgebung_t::background_color, dirty );
	}
}
