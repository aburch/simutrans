/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "network_address.h"
#include "network_packet.h"

#include <stdlib.h>
#include <string.h>


net_address_t::net_address_t(uint32 ip_, uint32 mask_) : ip(ip_), mask(mask_)
{
	ipstr[0] = '\0';
	init_ipstr();
}


net_address_t::net_address_t(const char *text)
{
	ipstr[0] = '\0';
	uint32 offset = 0;
	ip = 0;
	mask = 0;
	for(sint8 j=24; j>=0; j-=8) {
		uint32 n = atoi(text);
		ip   |= (n & 0xff) << j;
		mask |= 0xff << j;
		text = strchr(text+offset, '.');
		if (text) {
			text++;
		}
		else {
			break;
		}
	}
	init_ipstr();
}


void net_address_t::init_ipstr()
{
	if (  ipstr[0] == '\0'  ) {
		sprintf( ipstr, "%i.%i.%i.%i", (ip >> 24) & 255, (ip >> 16) & 255, (ip >> 8) & 255, ip & 255 );
	}
}


const char* net_address_t::get_str () const
{
	return ipstr;
}


void address_list_t::rdwr(packet_t *packet)
{
	uint32 count = get_count();
	packet->rdwr_long(count);
	for(uint32 i=0; i<count; i++) {
		if (packet->is_loading()) {
			append(net_address_t());
		}
		(*this)[i].rdwr(packet);
	}
}
