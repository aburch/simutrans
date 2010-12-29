
#include "../simdebug.h"
#include "network_packet.h"
#include "network_socket_list.h"


void packet_t::rdwr_header()
{
	rdwr_short( size );
	rdwr_short( version );
	rdwr_short( id );
	if (version > NETWORK_VERSION) {
		error = true;
	}
}

packet_t::packet_t() : memory_rw_t(buf,MAX_PACKET_LEN,true),
	size(0),
	version(NETWORK_VERSION),
	id(0),
	sock(INVALID_SOCKET),
	error(false),
	ready(false),
	count(0)
{
	set_index(HEADER_SIZE);
}

packet_t::packet_t(const packet_t &p) : memory_rw_t(buf,MAX_PACKET_LEN,true)
{
	version = p.version;
	id      = p.id;
	error   = p.error;
	ready   = p.ready;
	sock  = INVALID_SOCKET;
	size  = 0;
	count = 0;
	uint16 index = p.get_current_index();
	for(uint16 i = 0; i<index; i++) {
		buf[i] = p.buf[i];
	}
	set_index(index);
}

packet_t::packet_t(SOCKET sender) : memory_rw_t(buf,MAX_PACKET_LEN,false)
{
	// initialize data
	error = ( sender==INVALID_SOCKET );
	ready = false;
	version = 0;
	count = 0;
	size = 0;
	id = 0;
	version = 0;
	sock = sender;
}


void packet_t::recv()
{
	if (error  ||  ready) {
		return;
	}
	uint16 received = 0;
	// receive header
	if (count < HEADER_SIZE) {
		if (!network_receive_data(sock, buf + count, HEADER_SIZE - count, received, 0)) {
			error = true;
			return;
		}
		count += received;
		if (count == HEADER_SIZE) {
			// read header
			set_max_size(HEADER_SIZE);
			set_index(0);
			rdwr_header();

			if (size < HEADER_SIZE  ||  size > MAX_PACKET_LEN) {
				dbg->warning("packet_t::recv", "packet from [%d] has wrong size (%d)", sock, size);
				error = true;
				return;
			}
		}
		else {
			return;
		}
	}
	if (count >= HEADER_SIZE) {
		received = 0;
		if (!network_receive_data(sock, buf + count, size - count, received, 0)) {
			error = true;
			return;
		}
		count += received;
		if (count == size) {
			set_max_size(size);
			ready = true;
		}
	}
}


void packet_t::send(SOCKET s)
{
	if (has_failed()) {
		return;
	}
	// header written ?
	if (size == 0) {
		size = get_current_index();
		// write header at right place
		set_index(0);
		set_max_size(HEADER_SIZE);
		rdwr_header();
	}

	while (count < size) {
		int sent = ::send(s, (const char*) buf+count, size-count, 0);
		if (sent == -1) {
			int err = GET_LAST_ERROR();
			if (err != EWOULDBLOCK) {
				dbg->warning("packet_t::send", "error %d while sending to [%d]", err, s);
				error = true;
				return;
			}
			// try again later
			return;
		}
		if (sent == 0) {
			// connection closed
			dbg->warning("packet_t::send", "connection [%d] already closed", s);
			error = true;
			return;
		}
		count += sent;
		dbg->message("packet_t::send", "sent %d bytes to socket[%d]; id=%d, size=%d, left=%d", count, s, id, size, size-count);
	}

	// ready ?
	if (count == size) {
		ready = true;
		dbg->message("packet_t::send", "sent %d bytes to socket[%d]; id=%d, size=%d", count, s, id, size);
	}
}
