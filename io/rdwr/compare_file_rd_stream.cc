/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "compare_file_rd_stream.h"

#include <cassert>
#include <cstring>



compare_file_rd_stream_t::compare_file_rd_stream_t(rdwr_stream_t *s1, rdwr_stream_t* s2) : rdwr_stream_t(false)
{
	assert(s1  &&  s1->is_reading()  &&  s1->get_status() == STATUS_OK);
	assert(s2  &&  s2->is_reading()  &&  s2->get_status() == STATUS_OK);
	stream1 = s1;
	stream2 = s2;
	status = STATUS_OK;
	our_buf = NULL;
	our_len = 0;
}


compare_file_rd_stream_t::~compare_file_rd_stream_t()
{
	free(our_buf);
}


size_t compare_file_rd_stream_t::read(void *buf, size_t len)
{
	if (len > our_len) {
		free(our_buf);
		our_buf = (char*)malloc(len);
		our_len = len;
	}

	assert(!is_writing());

	size_t r1 = stream1->read(buf, len);
	size_t r2 = stream2->read(our_buf, len);

	bool ok = (r1 == r2)  &&  memcmp(buf, our_buf, r1)==0;

	// dump if difference is found
	if (!ok) {
		printf("File 1: ");
		for(int i=0; i<min(r1, 256); i++) {
			printf("0x%02x ", ((char*)buf)[i]);
		}
		printf("\n");
		printf("File 2: ");
		for(int i=0; i<min(r2, 256); i++) {
			printf("0x%02x ", our_buf[i]);
		}
		printf("\n");
	}

	assert(ok);
	return r1;
}
