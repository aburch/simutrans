#ifndef gui_messagebox_h
#define gui_messagebox_h

#include "base_info.h"
#include "components/gui_location_view_t.h"
#include "components/gui_image.h"
#include "../simcolor.h"

/**
 * A class for Message/news window.
 * @author Hj. Malthaner
 */
class news_window : public base_infowin_t
{
public:
	virtual PLAYER_COLOR_VAL get_titelcolor() const { return color; }

protected:
	news_window(const char* text, PLAYER_COLOR_VAL color);

private:
	PLAYER_COLOR_VAL color;
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
	news_img(const char* text, image_id bild, PLAYER_COLOR_VAL color=WIN_TITEL);

private:
	void init(image_id bild);
	gui_image_t bild;
};


/**
 * Shows a news window with a view on some location
 */
class news_loc : public news_window
{
public:
	news_loc(const char* text, koord k, PLAYER_COLOR_VAL color = WIN_TITEL);

	void map_rotate90( sint16 new_ysize );

	virtual koord3d get_weltpos(bool);

private:
	location_view_t view;
};

#endif
