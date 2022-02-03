/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "checksum.h"

#include <string.h>
#include <stdio.h>
#include "../simdebug.h"

checksum_t::checksum_t()
{
	sha = NULL;
	reset();
}

checksum_t& checksum_t::operator=(const checksum_t& other)
{
	assert(other.valid);
	valid = true;
	delete sha;
	sha = NULL;
	for(uint8 i=0; i<20; i++) {
		message_digest[i] = other.message_digest[i];
	}
	return *this;
}

checksum_t::checksum_t(const checksum_t &other)
{
	assert(other.valid);
	valid = true;
	sha = NULL;
	for(uint8 i=0; i<20; i++) {
		message_digest[i] = other.message_digest[i];
	}
}


checksum_t::~checksum_t()
{
	delete sha;
}

void checksum_t::reset()
{
	if (sha == NULL) {
		sha = new SHA1();
	}
	else {
		sha->Reset();
	}
	valid = false;
}

bool checksum_t::operator== (checksum_t &other)
{
	if (!valid) {
		finish();
	}
	if (!other.valid) {
		other.finish();
	}
	for(uint8 i=0; i<20; i++) {
		if (message_digest[i] != other.message_digest[i]) {
			return false;
		}
	}
	return true;
}

bool checksum_t::operator== (const checksum_t &other) const
{
	assert(valid && other.valid);
	for(uint8 i=0; i<20; i++) {
		if (message_digest[i] != other.message_digest[i]) {
			return false;
		}
	}
	return true;
}

void checksum_t::input(bool data)
{
	assert(sha);
	// save bool as (uint8)1
	uint8 bool1 = data ? 1 : 0;
	input(bool1);
}


void checksum_t::input(uint8 data)
{
	assert(sha);
	sha->Input((const char*)&data, sizeof(uint8));
}


void checksum_t::input(sint8 data)
{
	input((uint8)data);
}


void checksum_t::input(uint16 data)
{
	uint16 little_endian = endian(data);
	assert(sha);
	sha->Input((const char*)&little_endian, sizeof(uint16));
}


void checksum_t::input(sint16 data)
{
	input((uint16)data);
}


void checksum_t::input(uint32 data)
{
	uint32 little_endian = endian(data);
	assert(sha);
	sha->Input((const char*)&little_endian, sizeof(uint32));
}


void checksum_t::input(sint32 data)
{
	input((uint32)data);
}


void checksum_t::input(const char *data)
{
	if (data==NULL) {
		data = "null";
	}
	assert(sha);
	sha->Input(data, strlen(data));
}


const char* checksum_t::get_str(const int maxlen) const
{
	static char buf[41];
	for(uint8 i=0; i<min(maxlen,20); i++) {
		sprintf(buf + 2*i, "%2.2X", message_digest[i]);
	}
	return buf;
}


void checksum_t::calc_checksum(checksum_t *chk) const
{
	for(uint8 i=0; i<20; i++) {
		chk->input(message_digest[i]);
	}
}

void checksum_t::finish()
{
	if (!valid) {
		assert(sha);
		sha->Result(message_digest);
		valid = true;
		delete sha;
		sha = NULL;
	}
}
