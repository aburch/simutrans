/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

// This file should not be compiled. It wont compile either.

/** @file api_base.h documents base classes that are defined elsewhere. */

#ifdef DOXYGEN

/**
 * Translatable text.
 *
 * Class takes raw string. It can do variable substitution.
 *
 * When transferred to simutrans:
 * -# the translation of the string will be fetched
 * -# variables will be substituted
 *
 * Translated strings will be taken from tab-files inside the scenario_name/ directory.
 *
 * Usage:
 * @code
 * local text = ttext("You have {N} trains running.")
 * text.N = 20;
 * // if returned to simutrans, the output will be "You have 20 trains running."
 * @endcode
 */
class ttext { // begin_class("ttext", 0)

	/**
	 * Constructor.
	 * @param text raw string to be translated.
	 * @typemask (string)
	 */
	register_function(vm,, "constructor");

	/**
	 * Does translation and variable substitution.
	 * @returns Translated string with all variables replaced by their values.
	 * @typemask string()
	 */
	register_function("to_string");

}; // end_class

/**
 * Loads text from a file.
 *
 * It does variable substitution as ttext.
 */
class ttextfile : public ttext { // begin_class("ttextfile", "ttext")
	/**
	* Constructor. Text is taken from a file.
	*
	* The file is searched in the following paths:
	* -# scenario_name/iso/ .. iso is language ISO abbreviation
	* -# scenario_name/en/ .. fall-back: English version
	* -# scenario_name/
	*
	* @param file filename
	* @typemask (string)
	*/
	register_function(vm,, "constructor");
}; // end_class

/**
 * Class that implements an extended get-method.
 *
 * If child class has method
 * @code
 * function get_something()
 * @endcode
 * then function result can be accessed by
 * @code
 * local result = some_instance.something
 * @endcode
 * as if the class has a member variable.
 *
 *
 * Example:
 *
 * @code
 * local player = player_x(0)
 * local cash1 = player.get_cash()[0]
 * local cash2 = player.cash[0]
 * // now cash1 == cash2
 * @endcode
 */
class extend_get { // begin_class("extend_get", 0)
}; // end_class
#endif
