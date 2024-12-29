/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

 /** @file simutrans_doc.h Just contains pages of exciting documentation */


/**
 * @mainpage
 *
 * Simutrans
 *
 * @tableofcontents
 *
 *
 *
 *
 *
 * ## squirrel Scripting language
 * @section s_squirrel
 * ------------------
 *
 * The scripts have to be written in squirrel. The manual can be found at <a href="http://squirrel-lang.org/">Squirrel main page</a>.
 * As squirrels like to crack nuts, understandably the script files get the extension '.nut'.
 *
 * @subsection sec_scenarios Scripted scenarios
 *
 * ....
 *
 * ### How to create a scenario.
 *
 * You first need an @e idea - a vision what a scenario may look like. Then you have
 * to cast this idea into a @e savegame. That is, create the world in which your scenario will live.
 * You are the ruler of this toy universe, you are in charge of the rules, which go into the @e script.
 *
 * ### Recommended directory structure
 *
 * The scenario plays in a savegame. This savegame is tied to the pak-set you are using (e.g. pak64, pak128.Britain).
 * Hence, the scenario files have to go into a sub-folder of the pak-set.
 * The pak-set is found in a directory named pak-something, which is in the same directory, where the program
 * itself is located or under user-directory/addons/.
 *
 * Your scenario file goes into the folder
 *
 * <tt>
 * pak-something/scenario/myscenario/
 * </tt>
 *
 * Scenarios can also be put into the addons folder:
 *
 * <tt>
 * addons/pak-something/scenario/myscenario/
 * </tt>
 *
 *
 * @subsection sec_ai_player Scripted AI players
 *
 * ....
 *
 * ### Recommended directory structure
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
 * @subsection sec_scripted_tools Scripted tools
 *
 * ....
 *
 * ### Recommended directory structure
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
*
 */



/**
 * @defgroup squirrel-scen-api Squirrel scenario interface
 *
 * The following methods are vital for the functioning of a scripted scenarios.
 * They will be called from simutrans to interact with the script. You should consider
 * implementing them.
 *
 */

 /**
  * @defgroup squirrel-ai-api Squirrel ai interface
  *
  * The following methods are vital for the functioning of a scripted AI.
  * They will be called from simutrans to interact with the script. You should consider
  * implementing them.
  *
  */

  /**
   * @defgroup squirrel-tool-api Squirrel tool interface
   *
   * The following methods are vital for the functioning of scripted tools.
   * They will be called from simutrans to interact with the script. You should consider
   * implementing them.
   *
   */

   /**
  * @defgroup squirrel-toolkit-api Squirrel toolkit interface
  *
  * The following methods create macro scripted tools.
  *
  *
  *
  */
