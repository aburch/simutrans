/*
 * stadt_info.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef message_info_h
#define message_info_h


#include "gui_frame.h"
#include "components/gui_world_view_t.h"
#include "components/gui_image.h"
#include "components/gui_textarea.h"
#include "../utils/cbuffer_t.h"

class karte_t;


/**
 * Display a message on a non-world object
 * @author prissi
 */
class message_info_t : public gui_frame_t
{
private:
	gui_image_t bild;
	gui_textarea_t meldung;
	world_view_t view;

public:
	message_info_t(karte_t *welt, sint16 color, char *text, koord k, image_id bild);
};


#endif //message_frame_h
