/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_NETWORK_PACKET_H
#define NETWORK_NETWORK_PACKET_H


#include "../simtypes.h"
#include "memory_rw.h"
#include "network.h"

#define MAX_PACKET_LEN (8192)

// static const do not work on all compilers/architectures
#define HEADER_SIZE (6) // the network sizes are given ...


class packet_t : public memory_rw_t {
private:
	// the buffer
	uint8 buf[MAX_PACKET_LEN];
	// the header
	// [0]  size
	uint16 size;
	// [2]  version
	uint16 version;
	// [4]  id
	uint16 id;

	// who sent this packet
	SOCKET sock;

	bool error:1;
	// ready for sending / fully received
	bool ready:1;
	// how much already sent / received
	uint16 count;


	void rdwr_header();

public:
	/**
	 * constructor: packet is in saving-mode
	 */
	packet_t();
	packet_t(const packet_t &p);

	/**
	 * constructor: packet is in loading-mode
	 * @param s socket from where the packet has to be received
	 */
	packet_t(SOCKET s);

	/**
	 * start/continue sending
	 * sets bools ready or error
	 * @param complete forces to send the complete packet
	 */
	void send(SOCKET s, bool complete);

	/**
	 * start/continue receiving
	 * sets bools ready or error
	 */
	void recv();

	bool has_failed() const { return error  ||  is_overflow();}
	void failed() { error = true; }
	bool is_ready() const { return ready; }

	// can we understand the received packet?
	bool check_version() const { return is_saving() || (version <= NETWORK_VERSION); }

	uint16 get_id() const { return id; }
	void set_id(uint16 id_) { id = id_; }

	SOCKET get_sender() { return sock; }

	/**
	 * mark this packet as sent by the server
	 * @see network_send_server
	 */
	void sent_by_server();
};
#endif
