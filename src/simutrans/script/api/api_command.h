/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SCRIPT_API_API_COMMAND_H
#define SCRIPT_API_API_COMMAND_H


#include "../api_param.h"
#include "../../dataobj/koord3d.h"
#include "../../utils/plainstring.h"

void export_commands(HSQUIRRELVM vm);

class koord3d;
class player_t;
class tool_t;

namespace script_api {

	/**
	 * Class to hold pointer to tool_t and to manage its default_param string.
	 */
	struct my_tool_t {
		tool_t *tool;
		plainstring default_param;

		my_tool_t(tool_t *tool_) : tool(tool_) { }
	};

	/*
	 * Return types for tool calls:
	 * - call_tool_init
	 * - call_tool_work
	 *
	 * Their param<>::push methods actually call work/init and suspend.
	 *
	 * To implement a tool, make a method returning call_tool_work:
	 *
	 * call_tool_work call_x_tool(parameters..)
	 * {
	 *      ....
	 *      return call_tool_work(tool, player, koord3d)
	 * }
	 *
	 * call_tool_init call_x_tool(parameters..)
	 * {
	 *      ....
	 *      return call_tool_init(tool, player)
	 * }
	 */
	class call_tool_base_t {
	protected:
		// tool to be called, can be NULL
		tool_t *tool;
		// create tool with this id, default_param, and flags if tool == NULL
		uint16 tool_id;
		plainstring default_param;
		uint8 flags;
		player_t *player;
		// error message when calling, returned by push
		const char* error;
	public:
		call_tool_base_t(tool_t* t, player_t* pl)
		: tool(t), tool_id(0), default_param(NULL), flags(0), player(pl), error(NULL)  { }

		call_tool_base_t(uint16 id, const char* dp, uint8 f, player_t* pl)
		: tool(NULL), tool_id(id), default_param(dp), flags(f), player(pl), error(NULL) { }

		call_tool_base_t(const char* err)
		: tool(NULL), tool_id(0), default_param(NULL), flags(0), player(NULL), error(err)  { }

		tool_t * create_tool();
	};

	class call_tool_init : public call_tool_base_t {

		friend struct param<call_tool_init>;
	public:
		call_tool_init(tool_t* t, player_t* pl)
		: call_tool_base_t(t, pl) { }

		call_tool_init(uint16 id, const char* dp, uint8 f, player_t* pl)
		: call_tool_base_t(id, dp, f, pl) { }

		call_tool_init(const char* err)
		: call_tool_base_t(err) { }
	};

	class call_tool_work : public call_tool_base_t {
		// parameters to call the work methods
		koord3d start;
		koord3d end;
		bool twoclick;

		friend struct param<call_tool_work>;
	public:
		call_tool_work(tool_t* t, player_t* pl, koord3d pos1)
		: call_tool_base_t(t, pl),
		  start(pos1), end(koord3d::invalid), twoclick(false) {
		}

		call_tool_work(tool_t* t, player_t* pl, koord3d pos1, koord3d pos2)
		: call_tool_base_t(t, pl),
		  start(pos1), end(pos2), twoclick(true) {
		}

		call_tool_work(uint16 id, const char* dp, uint8 f, player_t* pl, koord3d pos1)
		: call_tool_base_t(id, dp, f, pl),
		  start(pos1), end(koord3d::invalid), twoclick(false) {
		}

		call_tool_work(uint16 id, const char* dp, uint8 f, player_t* pl, koord3d pos1, koord3d pos2)
		: call_tool_base_t(id, dp, f, pl),
		  start(pos1), end(pos2), twoclick(true) {
		}

		call_tool_work(const char* err)
		: call_tool_base_t(err),
		  start(koord3d::invalid), end(koord3d::invalid), twoclick(false) {
		}
	};


	template<> struct param<call_tool_init> {
		/**
		 * Pretends to be a push method. BUT, actually,
		 * Calls tool->init (or sends to server & suspends the script).
		 * Returns error string at the end of the day.
		 */
		static SQInteger push(HSQUIRRELVM vm, call_tool_init v);
		// returns nothing sensible
		static const char* squirrel_type() { return "void"; }
	};

	template<> struct param<call_tool_work> {
		/**
		 * Pretends to be a push method. BUT, actually,
		 * Calls tool->work (or sends to server & suspends the script).
		 * Returns error string at the end of the day.
		 */
		static SQInteger push(HSQUIRRELVM vm, call_tool_work v);
		// returns strings
		static const char* squirrel_type() { return param<const char*>::squirrel_type(); }
	};

	/**
	 * Helper functions to rename objects.
	 * @param what indicates type (see tool_rename_t::init)
	 */
	call_tool_init command_rename(player_t *owner, char what, koord3d pos, const char* name);
	call_tool_init command_rename(player_t *owner, char what, uint32 id, const char* name);
};

#endif
