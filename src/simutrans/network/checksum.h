/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_CHECKSUM_H
#define NETWORK_CHECKSUM_H


#include "../utils/sha1.h"

class checksum_t
{
private:
	sha1_hash_t message_digest;
	bool valid:1;
	SHA1 *sha;
public:
	checksum_t();
	checksum_t& operator=(const checksum_t&);
	checksum_t(const checksum_t&);
	~checksum_t();
	/**
	 * fetches the result
	 */
	void finish();
	void reset();
	bool operator== (checksum_t &);
	bool operator== (const checksum_t &) const;

	bool is_valid() const { return valid; }

	void input(bool data);
	void input(uint8 data);
	void input(sint8 data);
	void input(uint16 data);
	void input(sint16 data);
	void input(uint32 data);
	void input(sint32 data);
	void input(const char *data);
	const char* get_str(const int maxlen=20) const;

	// templated to be able to read/write from/to loadsave_t and packet_t
	template<class rdwr_able> void rdwr(rdwr_able *file)
	{
		if(file->is_saving()) {
			if (!valid) {
				finish();
			}
		}
		else {
			valid = true;
			delete sha;
			sha = 0;
		}
		for(uint8 i=0; i<20; i++) {
			file->rdwr_byte(message_digest[i]);
		}
	}

	/**
	 * build checksum of checksums
	 */
	void calc_checksum(checksum_t *chk) const;
};

#endif
