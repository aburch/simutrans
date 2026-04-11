/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/**
 * Minimal MCP (Model Context Protocol) server implementation.
 *
 * Transport: raw TCP, newline-delimited JSON (NDJSON).
 * Each JSON-RPC 2.0 message is one line terminated by LF.
 *
 * Supported methods:
 *   initialize          → reply with server capabilities
 *   notifications/...   → no reply (notifications from client)
 *   tools/list          → return registered tools (see mcp_tools.cc)
 *   tools/call          → execute a tool (see mcp_tools.cc)
 *   ping                → return empty result
 */

#include "mcp_server.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#if !USE_WINSOCK
#	include <fcntl.h>
#	include <unistd.h>
#endif

#include "../simdebug.h"
#include "../simversion.h"
#include "../simworld.h"
#include "mcp_tools.h"

// ---- static member storage ----
SOCKET mcp_server_t::listen_sock = INVALID_SOCKET;
std::vector<mcp_server_t::mcp_connection_t *> mcp_server_t::connections;
karte_t *mcp_server_t::world = NULL;


// ---- platform helpers ----

static void mcp_set_nonblocking(SOCKET s)
{
#if USE_WINSOCK
	u_long mode = 1;
	ioctlsocket(s, FIONBIO, &mode);
#else
	int flags = fcntl(s, F_GETFL, 0);
	if (flags >= 0) {
		fcntl(s, F_SETFL, flags | O_NONBLOCK);
	}
#endif
}

void mcp_server_t::close_socket(SOCKET s)
{
	if (s != INVALID_SOCKET) {
#if USE_WINSOCK
		closesocket(s);
#else
		close(s);
#endif
	}
}


// ---- public API ----

bool mcp_server_t::init(uint16 port)
{
	if (listen_sock != INVALID_SOCKET) {
		dbg->warning("mcp_server_t::init", "already running");
		return false;
	}

#if USE_WINSOCK
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		dbg->warning("mcp_server_t::init", "WSAStartup failed");
		return false;
	}
#endif

	// Try IPv6 dual-stack first, fall back to IPv4
	SOCKET s = socket(AF_INET6, SOCK_STREAM, 0);
	bool use_ipv6 = (s != INVALID_SOCKET);

	if (!use_ipv6) {
		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s == INVALID_SOCKET) {
			dbg->warning("mcp_server_t::init", "socket() failed");
			return false;
		}
	}

	{
		int on = 1;
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));
	}

	if (use_ipv6) {
		// Disable IPV6_V6ONLY so IPv4 clients connect too (dual-stack)
		int off = 0;
		setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&off, sizeof(off));

		struct sockaddr_in6 addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin6_family = AF_INET6;
		addr.sin6_port   = htons(port);
		addr.sin6_addr   = in6addr_loopback;
		if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
			close_socket(s);
			// Fallback to IPv4
			use_ipv6 = false;
			s = socket(AF_INET, SOCK_STREAM, 0);
			if (s == INVALID_SOCKET) {
				dbg->warning("mcp_server_t::init", "socket() failed (ipv4 fallback)");
				return false;
			}
			int on = 1;
			setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));
		}
	}

	if (!use_ipv6) {
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family      = AF_INET;
		addr.sin_port        = htons(port);
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
			dbg->warning("mcp_server_t::init", "bind() failed on port %u (error %d)", port, (int)GET_LAST_ERROR());
			close_socket(s);
			return false;
		}
	}

	if (listen(s, 8) != 0) {
		dbg->warning("mcp_server_t::init", "listen() failed (error %d)", (int)GET_LAST_ERROR());
		close_socket(s);
		return false;
	}

	mcp_set_nonblocking(s);
	listen_sock = s;
	dbg->message("mcp_server_t::init", "MCP server listening on port %u", port);
	return true;
}


void mcp_server_t::shutdown()
{
	for (mcp_connection_t *c : connections) {
		close_socket(c->sock);
		delete c;
	}
	connections.clear();
	close_socket(listen_sock);
	listen_sock = INVALID_SOCKET;
}


void mcp_server_t::step(karte_t *welt)
{
	world = welt;
	if (listen_sock == INVALID_SOCKET) {
		return;
	}

	accept_new();

	// Process existing connections; collect dead ones
	std::vector<mcp_connection_t *> dead;
	for (mcp_connection_t *c : connections) {
		io_connection(c);
		if (c->sock == INVALID_SOCKET) {
			dead.push_back(c);
		}
	}
	for (mcp_connection_t *c : dead) {
		connections.erase(std::find(connections.begin(), connections.end(), c));
		delete c;
	}
}


// ---- private: network I/O ----

void mcp_server_t::accept_new()
{
	for (;;) {
		struct sockaddr_storage addr;
		socklen_t addrlen = sizeof(addr);
		SOCKET client = accept(listen_sock, (struct sockaddr *)&addr, &addrlen);
		if (client == INVALID_SOCKET) {
			break; // no more pending connections
		}
		mcp_set_nonblocking(client);
		// TCP_NODELAY for low latency
		{
			int on = 1;
			setsockopt(client, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(on));
		}
		connections.push_back(new mcp_connection_t(client));
		dbg->message("mcp_server_t::accept_new", "MCP client connected");
	}
}


void mcp_server_t::io_connection(mcp_connection_t *c)
{
	// --- read ---
	bool peer_closed = false;
	char buf[4096];
	for (;;) {
#if USE_WINSOCK
		int n = recv(c->sock, buf, sizeof(buf), 0);
#else
		ssize_t n = recv(c->sock, buf, sizeof(buf), 0);
#endif
		if (n > 0) {
			c->recv_buf.append(buf, n);
		}
		else if (n == 0) {
			// Half-close: peer finished sending. Process buffered data then close after flush.
			peer_closed = true;
			break;
		}
		else {
			break; // EWOULDBLOCK or transient error; stop reading for now
		}
	}

	// Guard: if recv_buf is unreasonably large (broken client), disconnect
	if (c->recv_buf.size() > 1024 * 1024) {
		dbg->warning("mcp_server_t::io_connection", "recv_buf overflow, closing client");
		close_socket(c->sock);
		c->sock = INVALID_SOCKET;
		return;
	}

	// Process complete lines
	for (;;) {
		size_t nl = c->recv_buf.find('\n');
		if (nl == std::string::npos) {
			break;
		}
		std::string line = c->recv_buf.substr(0, nl);
		c->recv_buf.erase(0, nl + 1);
		// strip trailing \r if present
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		if (!line.empty()) {
			handle_line(c, line);
		}
	}

	// --- write pending send_buf ---
	while (c->sock != INVALID_SOCKET && !c->send_buf.empty()) {
#if USE_WINSOCK
		int n = send(c->sock, c->send_buf.data(), (int)c->send_buf.size(), 0);
#else
		ssize_t n = send(c->sock, c->send_buf.data(), c->send_buf.size(), 0);
#endif
		if (n > 0) {
			c->send_buf.erase(0, n);
		}
		else {
			break; // EWOULDBLOCK or error
		}
	}

	// Close after all pending data flushed, if peer already closed
	if (peer_closed && c->send_buf.empty()) {
		dbg->message("mcp_server_t::io_connection", "MCP client disconnected");
		close_socket(c->sock);
		c->sock = INVALID_SOCKET;
	}
}


void mcp_server_t::handle_line(mcp_connection_t *c, const std::string &line)
{
	std::string method  = json_get_string(line, "method");
	std::string id_json = json_get_raw(line, "id");
	std::string params  = json_get_raw(line, "params");

	if (method.empty()) {
		dbg->warning("mcp_server_t::handle_line", "no method in JSON: %.200s", line.c_str());
		return;
	}

	std::string response = dispatch(method, id_json, params);
	if (!response.empty()) {
		c->send_buf += response;
		c->send_buf += '\n';
	}
}


// ---- private: dispatch ----

std::string mcp_server_t::dispatch(const std::string &method,
                                    const std::string &id_json,
                                    const std::string &params_json)
{
	// Notifications (no id) → no response
	if (id_json.empty() || id_json == "null") {
		dbg->message("mcp_server_t::dispatch", "notification: %s", method.c_str());
		return "";
	}

	// --- initialize ---
	if (method == "initialize") {
		const char *result =
			"{"
			"\"protocolVersion\":\"2024-11-05\","
			"\"serverInfo\":{"
				"\"name\":\"simutrans-otrp\","
				"\"version\":\"" VERSION_NUMBER "\""
			"},"
			"\"capabilities\":{"
				"\"tools\":{}"
			"}"
			"}";
		return make_response(id_json, result);
	}

	// --- ping ---
	if (method == "ping") {
		return make_response(id_json, "{}");
	}

	// --- tools/list ---
	if (method == "tools/list") {
		std::string body = "{\"tools\":" + mcp_tools::tools_list_json() + "}";
		return make_response(id_json, body);
	}

	// --- tools/call ---
	if (method == "tools/call") {
		std::string tool_name = json_get_string(params_json, "name");
		std::string args_json = json_get_raw(params_json, "arguments");
		if (args_json.empty()) args_json = "{}";
		std::string result_json = mcp_tools::tools_call(tool_name, args_json, world);
		// wrap in MCP content array
		std::string escaped_result = json_escape(result_json);
		std::string content = "{\"content\":[{\"type\":\"text\",\"text\":\""
		                    + escaped_result + "\"}]}";
		return make_response(id_json, content);
	}

	// --- unknown method ---
	return make_error(id_json, -32601, "method not found: " + method);
}


// ---- private: JSON helpers ----

std::string mcp_server_t::make_response(const std::string &id_json, const std::string &result_json)
{
	return std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_json + ",\"result\":" + result_json + "}";
}

std::string mcp_server_t::make_error(const std::string &id_json, int code, const std::string &message)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%d", code);
	return std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_json +
	       ",\"error\":{\"code\":" + buf + ",\"message\":\"" + json_escape(message) + "\"}}";
}

std::string mcp_server_t::json_escape(const std::string &s)
{
	std::string out;
	out.reserve(s.size());
	for (unsigned char c : s) {
		switch (c) {
			case '"':  out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case '\n': out += "\\n";  break;
			case '\r': out += "\\r";  break;
			case '\t': out += "\\t";  break;
			default:
				if (c < 0x20) {
					char hex[8];
					snprintf(hex, sizeof(hex), "\\u%04x", c);
					out += hex;
				} else {
					out += (char)c;
				}
		}
	}
	return out;
}

/**
 * Find the raw JSON value for the first occurrence of "key": in a JSON object.
 * Returns the raw JSON token/value (object, array, string, number, bool, null).
 * Handles one level of nesting for objects and arrays.
 * Not a full JSON parser – sufficient for well-formed JSON-RPC envelopes.
 */
std::string mcp_server_t::json_get_raw(const std::string &json, const std::string &key)
{
	// Search for  "key":
	std::string needle = "\"" + key + "\"";
	size_t pos = json.find(needle);
	if (pos == std::string::npos) {
		return "";
	}
	pos += needle.size();
	// skip whitespace and ':'
	while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
		++pos;
	}
	if (pos >= json.size() || json[pos] != ':') {
		return "";
	}
	++pos;
	while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) {
		++pos;
	}
	if (pos >= json.size()) {
		return "";
	}

	char first = json[pos];
	if (first == '{' || first == '[') {
		// Scan to matching bracket
		char open  = first;
		char close = (first == '{') ? '}' : ']';
		int depth = 0;
		size_t start = pos;
		bool in_str = false;
		for (; pos < json.size(); ++pos) {
			char c = json[pos];
			if (in_str) {
				if (c == '\\') { ++pos; continue; }
				if (c == '"')  { in_str = false; }
			} else {
				if (c == '"')  { in_str = true; }
				else if (c == open)  { ++depth; }
				else if (c == close) { --depth; if (depth == 0) { ++pos; break; } }
			}
		}
		return json.substr(start, pos - start);
	}
	else if (first == '"') {
		// String: find closing quote respecting escapes
		size_t start = pos;
		++pos;
		bool escaped = false;
		for (; pos < json.size(); ++pos) {
			if (escaped)      { escaped = false; continue; }
			if (json[pos] == '\\') { escaped = true; continue; }
			if (json[pos] == '"')  { ++pos; break; }
		}
		return json.substr(start, pos - start);
	}
	else {
		// Number, bool, null: scan until delimiter
		size_t start = pos;
		while (pos < json.size()) {
			char c = json[pos];
			if (c == ',' || c == '}' || c == ']' || c == ' ' || c == '\t' || c == '\n' || c == '\r') {
				break;
			}
			++pos;
		}
		return json.substr(start, pos - start);
	}
}

std::string mcp_server_t::json_get_string(const std::string &json, const std::string &key)
{
	std::string raw = json_get_raw(json, key);
	if (raw.size() < 2 || raw.front() != '"' || raw.back() != '"') {
		return raw; // not a JSON string literal, return as-is
	}
	// Unescape basic sequences
	std::string out;
	out.reserve(raw.size());
	for (size_t i = 1; i + 1 < raw.size(); ++i) {
		if (raw[i] == '\\' && i + 2 < raw.size()) {
			++i;
			switch (raw[i]) {
				case '"':  out += '"';  break;
				case '\\': out += '\\'; break;
				case '/':  out += '/';  break;
				case 'n':  out += '\n'; break;
				case 'r':  out += '\r'; break;
				case 't':  out += '\t'; break;
				default:   out += raw[i]; break;
			}
		} else {
			out += raw[i];
		}
	}
	return out;
}
