/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/** @file api_doc.h Just contains pages of exciting documentation */

/**
 * @mainpage
 *
 * Simutrans offers to possibility to add scripted scenarios and computer-controlled players.
 *
 * @tableofcontents
 *
 * Scripting language
 * ------------------
 *
 * The scripts have to be written in squirrel. The manual can be found at <a href="http://squirrel-lang.org/">Squirrel main page</a>.
 * As squirrels like to crack nuts, understandably the script files get the extension '.nut'.
 *
 * @section s_scenarios Scripted scenarios
 *
 * See also @ref scen_skel and @ref scen_only.
 *
 * @subsection sec_howto How to create a scenario.
 *
 * You first need an @e idea - a vision what a scenario may look like. Then you have
 * to cast this idea into a @e savegame. That is, create the world in which your scenario will live.
 * You are the ruler of this toy universe, you are in charge of the rules, which go into the @e script.
 *
 *
 * @subsection sec_ex Example scenario scripts
 *
 * There are several commented examples.
 * -# @ref page_script_mill
 * -# @ref page_script_pharm
 *
 * @subsection sec_dir Recommended directory structure
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
 * If you need more complex texts, these go into sub-directories named after the language, in which they are written.
 *
 * <tt>
 * pak-something/scenario/myscenario/en/ <- English files go here.
 *
 * pak-something/scenario/myscenario/de/ <- German files go here.
 * </tt>
 *
 * Scenarios can also be put into the addons folder:
 *
 * <tt>
 * addons/pak-something/scenario/myscenario/
 * </tt>
 *
 *
 * @section s_ai_player Scripted AI players
 *
 * See also @ref ai_skel.
 *
 * @subsection sec_dir Recommended directory structure
 *
 * Your script file goes into the folder
 *
 * <tt>
 * ai/myai/
 * </tt>
 *
 * The main script file must be
 *
 * <tt>
 * ai/myai/ai.nut
 * </tt>
 *
 * The ai scripts can also be put into
 *
 * <tt>
 * addons/ai/myai/
 * </tt>
 *
 * @section s_scripted_tools Scripted tools
 *
 * See also @ref tool_skel and @ref tool_only.
 *
 * @subsection sec_dir_tools Recommended directory structure
 *
 * The script file (tool.nut) as well as the configuration file (description.tab) go into
 *
 * <tt>
 * pak-something/tool/mytool/
 * </tt>
 *
 * Related pak-files have to be placed in
 *
 * <tt>
 * pak-something/
 * </tt>
 *
 * directly.
 *
 * @subsection sec_description_tab The file description.tab
 *
 * This is a plain text file with the following entries:<br>
 * <tt>
 * title=Name of tool to be shown in tool selection dialog <br>
 * type=one_click or two_click<br>
 * tooltip=Tooltip for the icon in the toolbar<br>
 * restart=Set to 1 if the virtual machine has to be restarted after each call to the work functions.<br>
 * menu=Parameter that can be used to load tools with menuconf.tab<br>
 * icon=Name of cursor object (loaded from some pak-file), used images: 0 = cursor, 1 = icon, 2 = marker image<br>
 * </tt>
 *
 * @section s_general_advice General scripting advice
 *
 * Check out the sections on the <a href="modules.html">Modules</a> page.
 * See also @ref pitfalls.
 *
 * @subsection sec_data_types Squirrel data types
 *
 * Squirrel has some built-in data types: among them integer, string, tables, and arrays. These are
 * widely used throughout this documentation in the function declarations.
 *
 * If tables or arrays carry template parameters, this indicates the expected type of elements of the array or table.
 * See factory_x::input for an example.
 *
 * @subsection sec_coord Coordinates
 *
 * All coordinates in the script are with respect to the initial rotation of the map. If a player rotates
 * a map, then all coordinates are translated to the original rotation.
 * This affects the classes @ref coord and @ref coord3d as well as any functions that expect coordinates as input, as
 * for instance factory_x::factory_x or rules::forbid_way_tool_rect.
 *
 * @subsection sec_network Network play
 *
 * A lot of stuff is possible for network play. There are some limitations of course. These are due to the fact, that
 * the scripts are solely run on the server, the client does not need to have the script available at all.
 * This implies for instance that all scenario texts are translated to the server's language.
 *
 * @subsection sec_err Logging and error handling
 * If simutrans is started with '-debug 2', all errors and warnings are logged to standard output (i.e. terminal).
 * If simutrans is started in addition with '-log', all the output is written to the file script.log. In particular,
 * everything that is print-ed by the script, goes into this file.
 *
 * In case of error, an error window pops up showing the call stack and the values of local variables.
 * You can then repair your script, and restart the scenario via New Game - Scenario. You do not need to restart
 * simutrans to reload your script.
 *
 * @subsection sec_rdwr Load-Save support
 * Such support is available of course. If a running scenario is saved, the information about the scenario is saved within the
 * savegame. Upon loading, the scenario is resumed, by calling the function ::resume_game.
 *
 * You can save data in the savegame. In order to do so, you have to keep these data in the global table ::persistent.
 * This table is saved and loaded.
 *
 *
 */

/**
 * @example millionaire/scenario.nut
 * This example is commented in detail at the page @ref page_script_mill.
 */

/**
 * @page page_script_mill A simple example script
 *
 * Let us inspect a sample script file. We will have a look into the pak64 Millionaire scenario.
 * The objective of this scenario is to get rich as fast as possible.
 *
 * @dontinclude millionaire/scenario.nut
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
 * @example pharmacy-max/scenario.nut
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
 * @dontinclude pharmacy-max/scenario.nut
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
 * That's it. The remaining parts of this script are plain routine.
 */

/**
 * @page pitfalls Common pitfalls
 *
 * @subsection pitfall_pass_by_reference Tables, arrays, strings, classes, instances etc are passed by reference
 *
 * @code
 * local a = {}  // create an empty table
 * local b = a   // b refers to the same table as a
 * a.x <- 1      // b.x is now also 1
 * b.x <- 2      // a.x is changed to 2
 * @endcode
 *
 *
 * @subsection nocalls_in_global_scope Do not call API-functions in global scope in the script
 *
 * @code
 * local start_time = settings.get_start_time() // this call is executed BEFORE the savegame is loaded -> undefined
 *
 * local factory_coalmine = null                // we can create the variable ...
 * function start()
 * {
 *        factory_coalmine = factory_x(40, 78)  // ... but we can initialize it safely only in a function
 * }
 * @endcode
 *
 */

/**
 * @defgroup scen_skel Scenario interface
 *
 * The following methods are vital for the functioning of a scripted scenario.
 * They will be called from simutrans to interact with the script. You should consider
 * implementing them.
 *
 */

/**
 * @defgroup scen_only Scenario only functions
 *
 * These classes and methods are only available for scripted scenarios.
 *
 */

/**
 * @defgroup ai_skel AI interface
 *
 * The following methods are vital for the functioning of a scripted AI.
 * They will be called from simutrans to interact with the script. You should consider
 * implementing them.
 *
 */

/**
 * @defgroup ai_only AI only functions
 *
 * These classes and methods are only available for scripted AI players.
 *
 */

/**
 * @defgroup tool_skel Tool interface
 *
 * The following methods are vital for the functioning of scripted tools.
 * They will be called from simutrans to interact with the script. You should consider
 * implementing them.
 *
 */

/**
 * @defgroup tool_only Tool only functions
 *
 * These classes and methods are only available for scripted scenarios.
 *
 */

/**
 * @defgroup game_cmd Functions to alter the state of the game and map
 *
 * The player parameter in these functions represents the player that executes the command
 * and pays for it. If the call is from an AI player then the parameter is set to player_x::self,
 * and it will be checked whether the
 * player is permitted to execute the command. Calls from scenario always pass this check.
 *
 * In network games, the script is suspended until the command is executed, which is transparent to the script.
 * Hence such commands cannot be called from within functions that should return immediately, see @ref quick_return_func.
 */

/**
 * @defgroup rename_func Functions to rename something in the game
 * @ingroup game_cmd
 *
 * In AI mode, renaming works only if the object is owned by the script's player.
 */

/**
 * @defgroup quick_return_func Functions that should return quickly.
 *
 * These functions are intended to quickly return a result. In network games, it is not
 * allowed to call any map-altering tools from within such a function, see the section on @ref game_cmd.
 */

/**
 * @page page_sqstdlib Available functions from the squirrel standard lib
 *
 * - Math Library, see http://squirrel-lang.org/squirreldoc/stdlib/stdmathlib.html#squirrel-api
 * - String Library, see http://squirrel-lang.org/squirreldoc/stdlib/stdstringlib.html#squirrel-api
 * - System Library: functions clock, time, date, see http://squirrel-lang.org/squirreldoc/stdlib/stdsystemlib.html#squirrel-api
 *
 */
