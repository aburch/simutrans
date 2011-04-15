#include "network_address.h"
#include "network_packet.h"

#include <string.h>

net_address_t::net_address_t(const char *text)
{
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

}

void net_address_t::rdwr(packet_t *packet)
{
	packet->rdwr_long(ip);
	packet->rdwr_long(mask);
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
