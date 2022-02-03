/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_NETWORK_ADDRESS_H
#define NETWORK_NETWORK_ADDRESS_H


#include "../simtypes.h"
#include "../tpl/vector_tpl.h"

class packet_t;

class net_address_t {
public:
	uint32 ip;
	uint32 mask;

private:
	char ipstr[16];

	/**
	 * Generate human readable representation of this IP address
	 */
	void init_ipstr();

public:
	net_address_t(uint32 ip_=0, uint32 mask_ = 0xffffffff);

	net_address_t(const char *);

	bool matches(const net_address_t &other) const {
		return (other.ip & mask)==(ip & mask);
	}

	template<class F> void rdwr(F *packet)
	{
		packet->rdwr_long(ip);
		packet->rdwr_long(mask);
		if (packet->is_loading()) {
			ipstr[0] = '\0';
			init_ipstr();
		}
	}

	/**
	 * Return human readable representation of this IP address
	 */
	const char* get_str () const;

	bool operator==(const net_address_t& other) const {
		return ip==other.ip  &&  mask == other.mask;
	}

	uint32 get_ip() const { return ip; }
};

class address_list_t : public vector_tpl<net_address_t> {
public:
	address_list_t() : vector_tpl<net_address_t>(10) {}

	bool contains(const net_address_t &other) {
		for(net_address_t const& i : *this) {
			if (i.matches(other)) {
				return true;
			}
		}
		return false;
	}

	void rdwr(packet_t *packet);
};
#endif
