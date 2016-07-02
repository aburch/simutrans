#ifndef _API_COMMAND_H_
#define _API_COMMAND_H_

#include "../api_param.h"
#include "../../dataobj/koord3d.h"
#include "../../utils/plainstring.h"

void export_commands(HSQUIRRELVM vm);

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
	 * New return types for tool calls:
	 * - call_tool_init
	 * - call_tool_work
	 *
	 * Their param<>::push methods actually call work/init and suspend.
	 *
	 * To implement a tool, make a method returning call_tool_work:
	 *
	 * call_tool_init_t call_x_tool(parameters..)
	 * {
	 * 	return call_tool_init_t(tool, player)
	 * 	.. or ..
	 *      return call_tool_work_t(tool, player, koord3d)
	 * }
	 */

	class call_tool_work {
		// tool to be called, can be NULL
		tool_t *tool;
		// create tool with this id, default_param, and flags if tool == NULL
		uint16 tool_id;
		plainstring default_param;
		uint8 flags;
		// parameters to call the work methods
		player_t *player;
		koord3d start;
		koord3d end;
		bool twoclick;

		friend struct param<call_tool_work>;
	public:
		call_tool_work(tool_t* t, player_t* pl, koord3d pos1)
		: tool(t), tool_id(0), default_param(NULL), flags(0),
		  player(pl), start(pos1), end(koord3d::invalid), twoclick(false) {
		}

		call_tool_work(tool_t* t, player_t* pl, koord3d pos1, koord3d pos2)
		: tool(t), tool_id(0), default_param(NULL), flags(0),
		  player(pl), start(pos1), end(pos2), twoclick(true) {
		}

		call_tool_work(uint16 id, plainstring dp, uint8 f, player_t* pl, koord3d pos1)
		: tool(NULL), tool_id(id), default_param(dp), flags(f),
		player(pl), start(pos1), end(koord3d::invalid), twoclick(false) {
		}

		call_tool_work(uint16 id, plainstring dp, uint8 f, player_t* pl, koord3d pos1, koord3d pos2)
		: tool(NULL), tool_id(id), default_param(dp), flags(f),
		player(pl), start(pos1), end(pos2), twoclick(false) {
		}

		tool_t * create_tool();
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
};

#endif
