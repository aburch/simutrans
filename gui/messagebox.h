#ifndef gui_messagebox_h
#define gui_messagebox_h

#include "gui_frame.h"
#include "components/gui_location_view_t.h"
#include "components/gui_image.h"
#include "components/gui_fixedwidth_textarea.h"
#include "../simcolor.h"
#include "../utils/cbuffer_t.h"

/**
 * Eine Klasse für Nachrichtenfenster.
 * @author Hj. Malthaner
 */
class news_window : public gui_frame_t
{
public:
	virtual PLAYER_COLOR_VAL get_titelcolor() const { return color; }

	// Knightly : to extend the window with an extra component in the upper right corner
	void extend_window_with_component(gui_komponente_t *const component, const koord size, const koord offset = koord(0,0));

protected:
	news_window(const char* text, PLAYER_COLOR_VAL color);

private:
	cbuffer_t buf;
	gui_fixedwidth_textarea_t textarea;
	PLAYER_COLOR_VAL color;
};


/* Shows a news window with an image */
class news_img : public news_window
{
public:
	news_img(const char* text);
	news_img(const char* text, image_id bild, PLAYER_COLOR_VAL color=WIN_TITEL);

private:
	void init(image_id bild);
	gui_image_t bild;
};


/* Shows a news window with a view on some location */
class news_loc : public news_window
{
public:
	news_loc(karte_t* welt, const char* text, koord k, PLAYER_COLOR_VAL color = WIN_TITEL);

	void map_rotate90( sint16 new_ysize );

	virtual koord3d get_weltpos(bool);

private:
	location_view_t view;
};

#endif
