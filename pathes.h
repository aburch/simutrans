/*
 *  Copyright (c) 1997 - 2003 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 *  Description:
 *      This header defines all pathes used be simutrans relative to the game
 *	directory.
 */

#ifndef __PATHES_H
#define __PATHES_H

/**
 * two defines for all pathes - if You want the root path,  use:
 *	#define _PATH ""
 *	#define _PATH_X ""
 * else use
 *	#define _PATH "somewhere"
 *	#define _PATH_X _PATH "/"
 *
 * @author Volker Meyer
 * @date  18.06.2003
 */

#define FONT_PATH	    "font"
#define FONT_PATH_X	    FONT_PATH "/"

#define PALETTE_PATH	    "palette"
#define PALETTE_PATH_X	    PALETTE_PATH "/"

#define SAVE_PATH	    "save"
#define SAVE_PATH_X	    SAVE_PATH "/"
#define SAVE_PATH_X_LEN (sizeof(SAVE_PATH_X) - 1)

#define SCRENSHOT_PATH	    "screenshot"
#define SCRENSHOT_PATH_X    SCRENSHOT_PATH "/"

#endif
