/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMLOADINGSCREEN_H
#define SIMLOADINGSCREEN_H


#include "simtypes.h"
#include "tpl/slist_tpl.h"

struct event_t;

/**
 * Implements the loading screen related routines, in the aim of  centralize
 * all its code and make it more modular, as it was scattered across all code
 * before.
 *
 * @note The functions are safe on non-initialized displays, it won't try to write
 * on a not existent buffer.
 */
class loadingscreen_t
{
private:
	const char *what, *info;
	uint32 progress, max_progress;
	int last_bar_len;
	bool show_logo;
	slist_tpl<event_t *> queued_events;

	// show the logo if requested and there
	void display_logo();

	// show everything but the logo
	void display();

public:
	loadingscreen_t( const char *what, uint32 max_progress, bool show_logo = false, bool continueflag = false );

	~loadingscreen_t();

	void set_progress( uint32 progress );

	void set_max( uint32 max ) { max_progress = max; }

	void set_info( const char *info ) { this->info = info; }

	void set_what( const char *what ) { this->what = what; }
};

#endif
