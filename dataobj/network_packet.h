#ifndef _NETWORK_PACKET_H
#define _NETWORK_PACKET_H

#include "../simtypes.h"
#include "../utils/memory_rw.h"
#include "network.h"

#define MAX_PACKET_LEN (8192)

// static const do not work on all compilers/architectures
#define HEADER_SIZE (6)	// the network sizes are given ...


class packet_t : public memory_rw_t {
private:
	// the buffer
	uint8 buf[MAX_PACKET_LEN];
	// [2]	version
	uint16 version;
	// [4]  id
	uint16 id;

	bool error:1;

	// who sent this packet
	SOCKET sock;

	void rdwr_header( uint16 &size );
	// ready for sending
	bool   ready;

public:
	packet_t() : memory_rw_t(buf+HEADER_SIZE,MAX_PACKET_LEN-HEADER_SIZE,true), version(NETWORK_VERSION), id(0), error(false), sock(INVALID_SOCKET), ready(false) {};

	// read the packet from the socket
	packet_t(SOCKET s);

	void send(SOCKET s);

	bool has_failed() const { return error  ||  is_overflow();}
	void failed() { error = true; }

	// can we understand the received packet?
	bool check_version() const { return is_saving() || (version <= NETWORK_VERSION); }

	uint16 get_id() const { return id; }
	void set_id(uint16 id_) { id = id_; }

	SOCKET get_sender() { return sock; }
};
#endif
