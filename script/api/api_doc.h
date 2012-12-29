/** @file api_doc.h Just contains pages of exciting documentation */

/**
 * @mainpage
 *
 * @section sec_howto How to create a scenario.
 *
 * You first need an @e idea - a vision what a scenario may look like. Then you have
 * to cast this idea into a @e savegame. That is, create the world in which your scenario will live.
 * You are the ruler of this toy universe, you are in charge of the rules, which go into the @e script.
 *
 *
 * @section sec_sq Scripting language
 *
 * The scripts have to be written in squirrel. The manual can be found at <a href="http://squirrel-lang.org/">Squirrel main page</a>.
 * As squirrels like to crack nuts, understandably the script files get the extension '.nut'.
 *
 * @section sec_ex Example script
 *
 * There are several commented examples.
 * -# @ref page_script_mill
 * -# @ref page_script_pharm
 *
 *
 * @section sec_data_types Squirrel data types
 *
 * Squirrel has some built-in data types: among them integer, string, tables, and arrays. These are
 * widely used throughout this documentation in the function declarations.
 *
 * If tables or arrays carry template parameters, this indicates the expected type of elements of the array or table.
 * See factory_x::input for an example.
 *
 * @section sec_coord Coordinates
 *
 * All coordinates in the script are with respect to the initial rotation of the map. If a player rotates
 * a map, then all coordinates are translated to the original rotation.
 * This effects the classes @ref coord and @ref coord3d as well as any functions that expect coordinates as input, as
 * for instance factory_x::factory_x or rules::forbid_way_tool_rect.
 *
 * @section sec_network Network play
 *
 * A lot of stuff is possible for network play. There are some limitations of course. These are due to the fact, that
 * the script is solely run on the server, the client does not need to have the script available at all.
 * This implies for instance that all scenario texts are translated to the server's language.
 *
 * @section sec_err Logging and error handling
 * If simutrans is started with '-debug 2', all errors and warnings are logged to standard output (i.e. terminal).
 * If simutrans is started in addition with '-log', all the output is written to the file script.log. In particular,
 * everything that is print-ed by the script, goes into this file.
 *
 * In case of error, an error window pops up showing the call stack and the values of local variables.
 * You can then repair your script, and restart the scenario via New Game - Scenario. You do not need to restart
 * simutrans to reload your script.
 *
 * @section sec_rdwr Load-Save support
 * Such support is available of course. If a running scenario is saved, the information about the scenario is saved within the
 * savegame. Upon loading, the scenario is resumed, by calling the function ::resume_game.
 *
 * You can save data in the savegame. In order to do so, you have to keep these data in the global table ::persistent.
 * This table is saved and loaded.
 *
 * @section sec_dir Recommended directory structure
 *
 * The scenario plays in a savegame. This savegame is tied to the pak-set you are using (e.g. pak64, pak128.Britain).
 * Hence, the scenario files have to go into a sub-folder of the pak-set.
 * The pak-set is found in a directory named pak-something, which is in the same directory, where the program
 * itself is located.
 *
 * Your scenario file goes into the folder
 *
 * <tt>
 * pak-something/scenario/myscenario/
 * </tt>
 *
 * The scenario script must be in the file
 *
 * <tt>
 * pak-something/scenario/myscenario/scenario.nut
 * </tt>
 *
 * Translation files go into the scenario sub-directory with the same name as your file (just without extension)
 *
 * <tt>
 * pak-something/scenario/myscenario/
 * </tt>
 *
 * If need more complex texts, these go into sub-directories named after the language, in which they are written.
 *
 * <tt>
 * pak-something/scenario/myscenario/en/ <- English files go here.
 *
 * pak-something/scenario/myscenario/de/ <- German files go here.
 * </tt>
 *
 */

/**
 * @example millionaire.nut
 * This example is commented in detail at the page @ref page_script_mill.
 */

/**
 * @page page_script_mill A simple example script
 *
 * Let us inspect a sample script file. We will have a look into the pak64 Millionaire scenario.
 * The objective of this scenario is to get rich as fast as possible.
 *
 * @dontinclude millionaire.nut
 * As first the savegame is specified:
 * @skipline map.file
 * Then we provide some meta-information about the ::scenario
 * @skipline scenario
 * @skipline scenario
 * @skipline scenario
 * Rules and goals are not that special. The texts returned in this functions
 * will be shown in the scenario info window. String constants with leading
 * '@' can span multiple lines.
 * @until }
 * @until }
 * @until }
 * The method presenting the result text is a little bit more advanced. If the
 * player has enough cash, he has won the scenario.
 * @until }
 * The get_cash() method accesses the player's bank account via player_x.cash, which
 * is equivalent to calling player_x.get_cash().
 * @until }
 * Last but not least, the function that computes the actual progress in the scenario.
 * If it returns a negative value, the player has lost. If the return value is larger than 100,
 * this is equivalent to winning the scenario.
 * @until }
 */

/**
 * @example pharmacy-max.nut
 * This example is used to explain translatable texts at the page @ref page_script_pharm.
 */

/**
 * @page page_script_pharm A script showing translatable texts.
 *
 * Let us inspect another scenario script file. We will have a look into the pak64 Pharmacy-max scenario.
 * The objective is to get the pharmacy selling as much medicine as possible.
 *
 * We will learn how to make the translation of the script's output possible.
 *
 * @dontinclude pharmacy-max.nut
 * We do not care about the meta information, and jump right into the @ref get_rule_text method.
 * @skip get_rule_text
 * @until }
 * This function does not return a plain string, it sends the string into an instance of @ref ttext.
 * When the simutrans program now fetches the text, it will magically translate it.
 * This magic works only if there are translation files present.
 *
 * These files must be placed in a directory with the same name as the scenario, in our case at
 * <tt>
 * pharmacy-max/
 * </tt>
 * The translation files itself are plain text files with the naming convention <tt>iso.tab</tt>, where @em iso
 * refers to the ISO abbreviation of the language (en, de, cz etc).
 *
 * These text files have a simple line-oriented structure. Lets look into <tt>de.tab</tt>:
 *
 * @code
 * You can build everything you like. The sky (and your bank account) is the limit.
 * Alles ist erlaubt. Der Himmel (und das Bankkonto) ist die Grenze.
 * @endcode
 *
 * The first line is the string as it appears in the script, it will be mapped to the string in the following line.
 * A German user thus will read '<em>Alles ist erlaubt. Der Himmel (und das Bankkonto) ist die Grenze.</em>'.
 *
 * Similarly, the methods @ref get_info_text and @ref get_goal_text are implemented.
 * Some interesting stuff however happens in @ref get_result_text :
 * @skip get_result_text
 * @until }
 * Do you see the strange <tt>{med}</tt> string in the text? Here, you can see variable substitution in action:
 * @until text.med
 * That means the text now gets to know the precise value of <tt>{med}</tt>.
 *
 * When this text is transferred to simutrans (or when <tt>text.to_string()</tt> is called) the following happens:
 * -# First the translation of the string is searched, which can contain this <tt>{med}</tt> as well.
 * -# Then the occurrences of <tt>{med}</tt> are replaced by the concrete number.
 * The user will then hear '<em>The pharmacy sold 11 units of medicine per month.</em>'.
 *
 * Now a second string is created, which features some html-like tags.
 * @until }
 * Both of the strings are translated by means of calls to ttext::to_string and the result is returned.
 *
 * Thats it. The remaining parts of this script are plain routine.
 */

/**
 * @page deprecated_stuff Deprecated stuff
 *
 * @section sec_dir_112  Recommended directory structure (only simutrans nightly versions pre-r5989)
 *
 * The scenario plays in a savegame. This savegame is tied to the pak-set you are using (e.g. pak64, pak128.Britain).
 * Hence, the scenario files have to go into a sub-folder of the pak-set.
 * The pak-set is found in a directory named pak-something, which is in the same directory, where the program
 * itself is located.
 *
 * Your scenario file goes into the folder
 *
 * <tt>
 * pak-something/scenario/myscenario.nut
 * </tt>
 *
 * Translation files go into a sub-directory with the same name as your file (just without extension)
 *
 * <tt>
 * pak-something/scenario/myscenario/
 * </tt>
 *
 * If need more complex texts, these go into sub-directories named after the language, in which they are written.
 *
 * <tt>
 * pak-something/scenario/myscenario/en/ <- English files go here.
 *
 * pak-something/scenario/myscenario/de/ <- German files go here.
 * </tt>
 *
 */

/**
 * @defgroup post-112-1 Functions introduced after 112.1
 */
