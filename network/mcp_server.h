/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_MCP_SERVER_H
#define NETWORK_MCP_SERVER_H

#include "network.h"
#include <string>
#include <vector>

class karte_t;

/**
 * Minimal MCP (Model Context Protocol) server.
 * Listens on a TCP port, speaks JSON-RPC 2.0 over newline-delimited JSON.
 * step() must be called from the game main loop to process I/O on the main thread.
 */
class mcp_server_t
{
public:
	/**
	 * Start listening on the given port.
	 * @return true on success
	 */
	static bool init(uint16 port);

	/**
	 * Called once per game loop iteration (main thread).
	 * Accepts new connections, reads/writes data, dispatches requests.
	 * @param welt current world instance (may be NULL during startup)
	 */
	static void step(karte_t *welt);

	/** Close all connections and the listen socket. */
	static void shutdown();

	static bool is_active() { return listen_sock != INVALID_SOCKET; }

	// --- public JSON helpers (used by mcp_tools.cc) ---
	static std::string json_escape_pub(const std::string &s);
	static std::string json_get_raw_pub(const std::string &json, const std::string &key);
	static std::string json_get_string_pub(const std::string &json, const std::string &key);

private:
	struct mcp_connection_t {
		SOCKET sock;
		std::string recv_buf;
		std::string send_buf;
		bool initialized; ///< true after "initialize" handshake completed
		explicit mcp_connection_t(SOCKET s) : sock(s), initialized(false) {}
	};

	static SOCKET listen_sock;
	static std::vector<mcp_connection_t *> connections;
	static karte_t *world; ///< current world, set each step()

	static void accept_new();
	static void io_connection(mcp_connection_t *c);
	static void handle_line(mcp_connection_t *c, const std::string &line);

	/** Build a JSON-RPC success response. result_json is the raw JSON value. */
	static std::string make_response(const std::string &id_json, const std::string &result_json);

	/** Build a JSON-RPC error response. */
	static std::string make_error(const std::string &id_json, int code, const std::string &message);

	/** Dispatch a parsed request and return the response JSON string (empty = no response). */
	static std::string dispatch(const std::string &method,
	                            const std::string &id_json,
	                            const std::string &params_json);

	// --- internal JSON helpers (delegate to *_pub) ---
	static std::string json_escape(const std::string &s)
	         { return json_escape_pub(s); }
	static std::string json_get_raw(const std::string &json, const std::string &key)
	         { return json_get_raw_pub(json, key); }
	static std::string json_get_string(const std::string &json, const std::string &key)
	         { return json_get_string_pub(json, key); }

	static void close_socket(SOCKET s);
};

#endif // NETWORK_MCP_SERVER_H
