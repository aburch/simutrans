/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_IMAGE_LIST_H
#define GUI_COMPONENTS_GUI_IMAGE_LIST_H


#include "gui_action_creator.h"
#include "gui_component.h"
#include "../../tpl/vector_tpl.h"
#include "../../display/simimg.h"
#include "../../simcolor.h"

#define EMPTY_IMAGE_BAR (255)

/**
 * A component that represents a list of images.
 *
 * Updated! class is used only for the vehicle dialog. SO I changed some things
 * for the new one::
 * - cannot select no-image fields any more
 * - numbers can be drawn ontop an images
 * - color bar can be added to the images
 */
class gui_image_list_t :
	public gui_action_creator_t,
	public gui_component_t
{
public:
	struct image_data_t {
		const char *text;  ///< can be NULL, used to store external data
		image_id   image;  ///< the image
		sint16     count;  ///< display this number as overlay
		PIXVAL     lcolor; ///< color of left half of color bar, use EMPTY_IMAGE_BAR to display no bar
		PIXVAL     rcolor; ///< color of right half of color bar, use EMPTY_IMAGE_BAR to display no bar

		image_data_t(const char *text_, image_id image_, sint16 count_=0, PIXVAL lcolor_=EMPTY_IMAGE_BAR, PIXVAL rcolor_=EMPTY_IMAGE_BAR)
		: text(text_), image(image_), count(count_), lcolor(lcolor_), rcolor(rcolor_) {}
	};

	/**
	 * Graphic layout:
	 * size of borders around the whole area (there are no borders around
	 * individual images)
	 */
	enum {
		BORDER = 4
	};

private:
	vector_tpl<image_data_t*> *images;

	scr_coord grid;
	scr_coord placement;

	/**
	 * Player number to obtain player color used to display the images.
	 */
	sint8 player_nr;

	scr_coord_val max_width;

	sint32 max_rows;

public:
	/**
	 * Constructor: takes pointer to vector with image_data_t
	 * @param images pointer to vector of pointers to image_data_t
	 */
	gui_image_list_t(vector_tpl<image_data_t*> *images);

	/**
	 * This set horizontal and vertical spacing for the images.
	 */
	void set_grid(scr_coord grid) { this->grid = grid; }

	/**
	 * This set the offset for the images.
	 */
	void set_placement(scr_coord placement) { this->placement = placement; }

	void set_player_nr(sint8 player_nr) { this->player_nr = player_nr; }

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Draw/record the picture
	 */
	void draw(scr_coord offset) OVERRIDE;

	/**
	 * Looks for the image at given position.
	 * xpos and ypos relative to parent window.
	 */
	int index_at(scr_coord parent_pos, int xpos, int ypos) const;

	void set_max_rows(sint32 r) { max_rows = r; }

	void set_max_width(scr_coord_val w) { max_width = w; }

	scr_size get_min_size() const OVERRIDE;
	scr_size get_max_size() const OVERRIDE;
};

#endif
