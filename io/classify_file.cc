/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "classify_file.h"

#include "rdwr/bzip2_file_rdwr_stream.h"
#include "rdwr/raw_file_rdwr_stream.h"
#include "rdwr/zlib_file_rdwr_stream.h"
#ifdef USE_ZSTD
#include "rdwr/zstd_file_rdwr_stream.h"
#endif

#include "../dataobj/loadsave.h"
#include "../simdebug.h"
#include "../simversion.h"
#include "../sys/simsys.h"
#include "../utils/simstring.h"
#include "../macros.h"

#include <cstring>


bool classify_as_png(FILE *f, file_info_t *info);
bool classify_as_bmp(FILE *f, file_info_t *info);
bool classify_as_ppm(FILE *f, file_info_t *info);
bool classify_as_zstd(FILE *f, file_info_t *info);
bool classify_as_bzip2(FILE *f, file_info_t *info);
bool classify_as_zip(FILE *f, file_info_t *info);
bool classify_file_data(rdwr_stream_t *stream, file_info_t *info);


file_info_t::file_info_t() :
	file_type(TYPE_RAW),
	version(INVALID_FILE_VERSION),
	header_size(0)
{
	pak_extension[0] = 0;
}


file_info_t::file_info_t(file_type_t file_type, uint32 version) :
	file_type(file_type),
	version(version)
{
	pak_extension[0] = 0;
}


file_classify_status_t classify_save_file(const char *path, file_info_t *info)
{
	if (!info) {
		dbg->error("classify_file()", "Cannot classify file: info is NULL");
		return FILE_CLASSIFY_INVALID_ARGS;
	}
	else if(  !path  ||  !*path  ) {
		dbg->error("classify_file()", "Invalid path");
		return FILE_CLASSIFY_INVALID_ARGS;
	}

#ifdef MAKEOBJ
	FILE *f = fopen(path, "rb");
#else
	FILE *f = dr_fopen(path, "rb");
#endif

	if (!f) {
		// Do not warn about this since we can also use this function to check whether a file exists
		return FILE_CLASSIFY_NOT_EXISTING;
	}

#ifdef MAKEOBJ
	info->file_type = file_info_t::TYPE_RAW;
	info->version = INVALID_FILE_VERSION;
	info->header_size = 0;
	return FILE_CLASSIFY_OK;
#else

	fseek(f, 0, SEEK_SET);
	if (classify_as_zstd(f, info)) {
		fclose(f);

#if USE_ZSTD // otherwise we cannot read it
		zstd_file_rdwr_stream_t s(path, false, 0);
		if (!classify_file_data(&s, info)) {
#else
		{
#endif
			info->file_type = file_info_t::TYPE_ZSTD;
			info->version = INVALID_FILE_VERSION;
			info->header_size = 0;
		}

		return FILE_CLASSIFY_OK;
	}

	fseek(f, 0, SEEK_SET);
	if (classify_as_bzip2(f, info)) {
		fclose(f);

		bzip2_file_rdwr_stream_t s(path, false);
		if (!classify_file_data(&s, info)) {
			info->file_type = file_info_t::TYPE_RAW;
			info->version = INVALID_FILE_VERSION;
			info->header_size = 0;
		}

		return FILE_CLASSIFY_OK;
	}

	fseek(f, 0, SEEK_SET);
	if (classify_as_zip(f, info)) {
		fclose(f);

		zlib_file_rdwr_stream_t s(path, false, 0);
		if (!classify_file_data(&s, info)) {
			info->file_type = file_info_t::TYPE_RAW;
			info->version = INVALID_FILE_VERSION;
			info->header_size = 0;
		}

		return FILE_CLASSIFY_OK;
	}

	fseek(f, 0, SEEK_SET);

	raw_file_rdwr_stream_t s(f, false);
	if (!classify_file_data(&s, info)) {
		info->file_type = file_info_t::TYPE_RAW;
		info->version = INVALID_FILE_VERSION;
		info->header_size = 0;
	}

	return FILE_CLASSIFY_OK;
#endif // MAKEOBJ
}


file_classify_status_t classify_image_file(const char *path, file_info_t *info)
{
	if (!info) {
		dbg->error("classify_file()", "Cannot classify file: info is NULL");
		return FILE_CLASSIFY_INVALID_ARGS;
	}
	else if(  !path  ||  !*path  ) {
		dbg->error("classify_file()", "Invalid path");
		return FILE_CLASSIFY_INVALID_ARGS;
	}

#ifdef MAKEOBJ
	FILE *f = fopen(path, "rb");
#else
	FILE *f = dr_fopen(path, "rb");
#endif

	if (!f) {
		// Do not warn about this since we can also use this function to check whether a file exists
		return FILE_CLASSIFY_NOT_EXISTING;
	}

	fseek(f, 0, SEEK_SET);
	if (classify_as_png(f, info)) {
		fclose(f);
		return FILE_CLASSIFY_OK;
	}

	fseek(f, 0, SEEK_SET);
	if (classify_as_bmp(f, info)) {
		fclose(f);
		return FILE_CLASSIFY_OK;
	}

	fseek(f, 0, SEEK_SET);
	if (classify_as_ppm(f, info)) {
		fclose(f);
		return FILE_CLASSIFY_OK;
	}

	info->file_type = file_info_t::TYPE_RAW;
	info->version = INVALID_FILE_VERSION;
	info->header_size = 0;

	return FILE_CLASSIFY_OK;
}


bool classify_as_png(FILE *f, file_info_t *info)
{
	char buf[80];
	if (fread(buf, 1, 8, f) != 8) {
		return false;
	}

	if (memcmp(buf, "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a", 8) != 0) {
		return false;
	}

	info->file_type = file_info_t::TYPE_PNG;
	return true;
}


bool classify_as_bmp(FILE *f, file_info_t *info)
{
	char buf[80];
	if (fread(buf, 1, 2, f) != 2) {
		return false;
	}

	if (memcmp(buf, "BM", 2) != 0) {
		return false;
	}

	info->file_type = file_info_t::TYPE_BMP;
	return true;
}


bool classify_as_ppm(FILE *f, file_info_t *info)
{
	char buf[80];
	if (fread(buf, 1, 2, f) != 2) {
		return false;
	}

	if (memcmp(buf, "P6", 2) != 0) {
		return false;
	}

	info->file_type = file_info_t::TYPE_PPM;
	return true;
}

#ifndef MAKEOBJ
bool classify_as_bzip2(FILE *f, file_info_t *info)
{
	char buf[80];
	if (fread(buf, 1, 2, f) != 2) {
		return false;
	}

	if(  memcmp(buf, "BZ", 2) != 0) {
		return false; // not bzip2 compressed
	}

	info->file_type = file_info_t::TYPE_BZIP2;
	return true;
}


bool classify_as_zstd(FILE *f, file_info_t *info)
{
	char buf[80];
	if (fread(buf, 1, 2, f) != 2) {
		return false;
	}

	if(  memcmp(buf, "ZD", 2) != 0) {
		return false; // not zstd compressed
	}

	info->file_type = file_info_t::TYPE_ZSTD;
	return true;
}


bool classify_as_zip(FILE *f, file_info_t *info)
{
	char buf[80];
	if (fread(buf, 1, 2, f) != 2) {
		return false;
	}

	if(  memcmp(buf, "\x1f\x8b", 2) != 0  &&  buf[0] != 0x78  ) {
		return false; // not zlib/gzip compressed
	}

	info->file_type = file_info_t::TYPE_ZIPPED;
	return true;
}


bool classify_file_data(rdwr_stream_t *stream, file_info_t *info)
{
	char buf[80];
	MEMZERO(buf);

	if(  stream->read( buf, sizeof( SAVEGAME_PREFIX ) ) != sizeof( SAVEGAME_PREFIX )  ) {
		info->version = INVALID_FILE_VERSION;
		return false; // file too short
	}

	info->header_size = sizeof(SAVEGAME_PREFIX);

	// get the rest of the string
	for( int i = sizeof( SAVEGAME_PREFIX ); i < 79; ) {
		char ch;
		stream->read(&ch, 1);
		info->header_size++;
		if( ch < ' ' ) {
			break;
		}
		buf[ i++ ] = (char)ch;
		buf[ i ] = 0;
	}

	if (strstart(buf, SAVEGAME_PREFIX)) {
		info->version = loadsave_t::int_version(buf + sizeof(SAVEGAME_PREFIX) - 1, info->pak_extension);
		if(  info->version == 0  ) {
			info->version = INVALID_FILE_VERSION;
			return false;
		}
	}
	else if (strstart(buf, XML_SAVEGAME_PREFIX)) {
		info->file_type |= file_info_t::TYPE_XML;

		char ch;
		do {
			info->header_size++;
			stream->read(&ch, 1);
		} while (ch != '<');

		stream->read(buf, sizeof(SAVEGAME_PREFIX) - 1);
		info->header_size += sizeof(SAVEGAME_PREFIX) -1;

		if (!strstart(buf, SAVEGAME_PREFIX)) {
			// not a simutrans XML file ...
			info->version = INVALID_FILE_VERSION;
			return false;
		}

		stream->read(buf, sizeof("version=\"") - 1);
		info->header_size += sizeof("version=\"") - 1;

		char str[256];
		char *s = str;
		for (int i = 0; i < 255; i++) {
			char c;
			stream->read(&c, 1);
			info->header_size++;
			if (c=='\"') {
				break;
			}
			*s++ = c;
		}
		*s = 0;

		info->version = loadsave_t::int_version(str, info->pak_extension);
		if(  info->version == 0  ) {
			info->version = INVALID_FILE_VERSION;
			info->header_size = 0;
			return false;
		}

		stream->read(buf, sizeof(" pak=\"") - 1);
		info->header_size += sizeof(" pak=\"") - 1;

		if (info->version > 0) {
			s = info->pak_extension;
			for (int i = 0; i < 63; i++) {
				char c;
				stream->read(&c, 1);
				info->header_size++;

				if (c=='\"') {
					break;
				}
				*s++ = c;
			}
			*s = 0;

			char c;
			do {
				info->header_size++;
				stream->read(&c, 1);
			} while (c != '>');
		}
	}
	else {
		// not a Simutrans specific file
		info->version = INVALID_FILE_VERSION;
		info->header_size = 0;
		return false;
	}

	if(*info->pak_extension == 0) {
		strcpy( info->pak_extension, "(unknown)" );
	}

	return true;
}
#endif
