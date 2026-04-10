/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_MCP_TOOLS_H
#define NETWORK_MCP_TOOLS_H

#include <string>

class karte_t;

/**
 * MCP tool registry and dispatcher.
 * All tool handlers run on the main game thread (called from mcp_server_t::step).
 */
namespace mcp_tools {

/** JSON array string suitable for the "tools" field in tools/list response. */
std::string tools_list_json();

/**
 * Execute a tool call.
 * @param name      tool name (e.g. "get_time")
 * @param args_json raw JSON object string for the arguments (may be "{}" or "")
 * @param welt      current world pointer (may be null if world not loaded yet)
 * @return JSON-serialised result value (to be wrapped in MCP content array)
 */
std::string tools_call(const std::string &name,
                       const std::string &args_json,
                       karte_t           *welt);

} // namespace mcp_tools

#endif // NETWORK_MCP_TOOLS_H
