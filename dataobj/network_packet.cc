
#include "../simdebug.h"
#include "network_packet.h"


void packet_t::rdwr_header( uint16 &size )
{
	rdwr_short( size );
	rdwr_short( version );
	rdwr_short( id );
	if (version > NETWORK_VERSION) {
		error = true;
	}
}


packet_t::packet_t(SOCKET sender) : memory_rw_t(buf,MAX_PACKET_LEN,false)
{
	// initialize data
	error = false;
	ready = true;
	version = 0;
	uint16 size = 0;

	if(sender==INVALID_SOCKET  ) {
		error = true;
		return;
	}

	// read the header
	if(  network_recieve_data(sender,buf,HEADER_SIZE)!=HEADER_SIZE ) {
		error = true;
		network_remove_client( sender );
		return;
	}
	init( buf, HEADER_SIZE, false );
	rdwr_header( size );

	if (size > MAX_PACKET_LEN) {
		error = true;
		return;
	}

	// receive the rest of the packet
	if(  network_recieve_data( sender, buf+HEADER_SIZE, size-HEADER_SIZE )!=size-HEADER_SIZE  ) {
		error = true;
		network_remove_client( sender );
		return;
	}
	init( buf+HEADER_SIZE, size-HEADER_SIZE, false );

	// received succesfully, remember sender
	sock = sender;
}


void packet_t::send(SOCKET s)
{
	if (!has_failed()) {
		uint16 size = get_current_size()+HEADER_SIZE;

		if (!ready) {
			uint16 index = get_current_size();
			// write header at right place
			init( buf, HEADER_SIZE, true );
			rdwr_header( size );
			// reset values
			init( buf+HEADER_SIZE, MAX_PACKET_LEN-HEADER_SIZE, true );
			set_index(index);

			ready = true;
		}

		int sent = ::send(s, (const char*) buf, size, 0);
		dbg->message("packet_t::send", "sent %d bytes; id=%d, size=%d", sent, id, size);
		if (sent!=size) {
			error = true;
		}
	}
	else {
		dbg->error("packet_t::send", "error in creation!");
	}
}

