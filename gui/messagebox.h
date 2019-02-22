#ifndef gui_messagebox_h
#define gui_messagebox_h

#include "base_info.h"
#include "components/gui_location_view_t.h"
#include "components/gui_image.h"
#include "../simcolor.h"
#include "../dataobj/environment.h"

/**
 * A class for Message/news window.
 * @author Hj. Malthaner
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
class fatal_news : public news_window
{
public:
	fatal_news(const char* text);
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
