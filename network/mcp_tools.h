/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_MCP_TOOLS_H
#define NETWORK_MCP_TOOLS_H

#include <string>

class karte_t;
class script_vm_t;

/**
 * MCP tool registry and dispatcher.
 * All tool handlers run on the main game thread (called from mcp_server_t::step).
 */
namespace mcp_tools {

/** JSON array string suitable for the "tools" field in tools/list response. */
std::string tools_list_json();

/**
 * Execute a tool call.
 * @param name           tool name (e.g. "run_squirrel")
 * @param args_json      raw JSON object string for the arguments (may be "{}" or "")
 * @param welt           current world pointer (may be null if world not loaded yet)
 * @param out_pending_vm if non-null and the script suspends (network mode command_x),
 *                       the VM is stored here instead of being deleted; the caller
 *                       must resume and eventually delete it.  The return value is ""
 *                       in this case (response will be sent asynchronously).
 * @return JSON-serialised result value, or "" when *out_pending_vm was set.
 */
std::string tools_call(const std::string &name,
                       const std::string &args_json,
                       karte_t           *welt,
                       script_vm_t      **out_pending_vm = nullptr);

} // namespace mcp_tools

#endif // NETWORK_MCP_TOOLS_H
