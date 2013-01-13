/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef SIMLOADINGSCREEN_H
#define SIMLOADINGSCREEN_H

/**
 * Implements the loading screen related routines, in the aim of  centralize
 * all its code and make it more modular, as it was scattered across all code
 * before.
 * @author Markohs
 * @note Many of this functions are for internal use only, but I decided to
 * expose them all and keep them under the namespace to not clobber the global
 * namespace.
 * @note The functions are safe on non-initialized displays, it won't try to write
 * on a not existant buffer.
 */
namespace loadingscreen{
	/**
	 * Inits the namespace state
	 */
	void bootstrap();

	/**
	 * Shows the loading screen, if templated, won't do anything in the other case
	 */
	void show();

	/**
	 * Hides the loading screen, if templated, won't do anything in the other case
	 */
	void hide();

	/**
     * Updates the progress bar
	 * @param level Progress level to set.
	 * @param max Maximum expected value to level, the bar will be scaled proportionally to it.
	 * @note It will flush the screen buffer.
     */
	void set_progress(const unsigned int level,const unsigned int max);

	// "private" section:

	/**
	 * Will show the logo on screen.
	 */
	void show_logo();

	/**
	 * Takes care of system events, it's needed since the loading screen is allways called
	 * outside of the places where the program does this. In modal_dialogue and karte_t::interactive.
	 * @note Not doing this can cause the OS to identify our program as irresponsive.
	 */
	void handle_events();

	/**
	 * Shows the copyright string.
	 */
	void show_copyright();

	/**
     * Sets the loading bar message
	 * @param message The text to set. It's not copied so you can't free this memory after.
     */
	void set_label(const char *message);

	/**
     * Sets the copyright line
	 * @param message The text to set. It's not copied so you can't free this memory after.
     */
	void set_copyright(const char *message);

	/**
	 * Effect to use on background
	 */
	void shadow_screen();
};

#endif
