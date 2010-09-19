#ifndef _CHECKSUM_H_
#define _CHECKSUM_H_

#include "sha1.h"

class SHA1;
class loadsave_t;

class checksum_t
{
private:
	uint8 message_digest[20];
	bool valid:1;
	SHA1 *sha;
public:
	checksum_t();
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

	void rdwr(loadsave_t *file);

	/**
	 * build checksum of checksums
	 */
	void calc_checksum(checksum_t *chk) const;
};

#endif
