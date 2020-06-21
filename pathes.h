/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef PATHES_H
#define PATHES_H


/**
 * This header defines all paths used be simutrans relative to the game directory.
 *
 * two defines for all paths - if You want the root path,  use:
 *   #define _PATH ""
 *   #define _PATH_X ""
 * else use
 *   #define _PATH "somewhere"
 *   #define _PATH_X _PATH "/"
 */

#define FONT_PATH      "font"
#define FONT_PATH_X     FONT_PATH "/"

#define SAVE_PATH       "save"
#define SAVE_PATH_X     SAVE_PATH "/"
#define SAVE_PATH_X_LEN (sizeof(SAVE_PATH_X) - 1)

#define SCREENSHOT_PATH     "screenshot"
#define SCREENSHOT_PATH_X    SCREENSHOT_PATH "/"

#endif
