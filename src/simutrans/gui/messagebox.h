/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_MESSAGEBOX_H
#define GUI_MESSAGEBOX_H


#include "base_info.h"
#include "components/gui_location_view.h"
#include "components/gui_image.h"
#include "../simcolor.h"
#include "../dataobj/environment.h"

/**
 * A class for Message/news window.
 */
class news_window : public base_infowin_t
{
public:
	FLAGGED_PIXVAL get_titlecolor() const OVERRIDE { return color; }

protected:
	news_window(const char* text, FLAGGED_PIXVAL color);

private:
	FLAGGED_PIXVAL color;
};

/**
 * Displays fatal error message.
 */
class fatal_news : public news_window, private action_listener_t
{
	button_t copy_to_clipboard;

public:
	fatal_news(const char* text);

private:
	bool action_triggered(gui_action_creator_t *comp, value_t extra) OVERRIDE;
};


/**
 * Shows a news window with an image
 */
class news_img : public news_window
{
public:
	news_img(const char* text);
	news_img(const char* text, image_id image, FLAGGED_PIXVAL color = env_t::default_window_title_color);

private:
	void init(image_id image);
	gui_image_t image;
};


/**
 * Shows a news window with a view on some location
 */
class news_loc : public news_window
{
public:
	news_loc(const char* text, koord k, FLAGGED_PIXVAL color = env_t::default_window_title_color);

	void map_rotate90( sint16 new_ysize ) OVERRIDE;

	koord3d get_weltpos(bool) OVERRIDE;

private:
	location_view_t view;
};

#endif
