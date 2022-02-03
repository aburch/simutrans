/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef IO_CLASSIFY_FILE_H
#define IO_CLASSIFY_FILE_H


#include "../utils/unicode.h"
#include "../simtypes.h"


enum file_classify_status_t {
	FILE_CLASSIFY_OK = 0,
	FILE_CLASSIFY_INVALID_ARGS,
	FILE_CLASSIFY_NOT_EXISTING
};


#define INVALID_FILE_VERSION 0xFFFFFFFFu


/**
 * Holds information about file type, format and version.
 * Use @ref classify_save_file and @ref classify_image_file to classify
 * save files and image files, respectively.
 */
struct file_info_t
{
	enum file_type_t {
		TYPE_RAW = 0, // either raw binary or text (dat etc)

		TYPE_ZIPPED,  // zipped save
		TYPE_BZIP2,   // bzip2 compressed save
		TYPE_ZSTD,    // zstd compressed save

		TYPE_PNG,     // PNG image
		TYPE_BMP,
		TYPE_PPM,

		TYPE_XML        = 1u << 31,

		// Combined file formats
		TYPE_XML_ZIPPED = TYPE_XML | TYPE_ZIPPED,
		TYPE_XML_BZIP2  = TYPE_XML | TYPE_BZIP2,
		TYPE_XML_ZSTD   = TYPE_XML | TYPE_ZSTD
	};

public:
	file_info_t();
	explicit file_info_t(file_type_t file_type, uint32 version = INVALID_FILE_VERSION);

public:
	file_type_t file_type;
	uint32 version;         ///< Version of saved game.
	char pak_extension[64]; ///< Pak extension of saved game.
	size_t header_size;     ///< Header size of saved game in bytes
};
ENUM_BITSET(file_info_t::file_type_t);


/**
 * Classify a save file.
 * @param path must a valid system name, either a short name for windows or UTF8 for other plattforms
 * @param info If successfully classified, holds information about file format and version.
 *             Must not be NULL.
 * @returns FILE_CLASSIFY_OK iff successfully classified.
 */
file_classify_status_t classify_save_file(const char *path, file_info_t *info);

/**
 * Classify an image file.
 * @param path must a valid system name, either a short name for windows or UTF8 for other plattforms
 * @param info If successfully classified, holds information about file format and version.
 *             Must not be NULL.
 * @returns FILE_CLASSIFY_OK iff successfully classified.
 */
file_classify_status_t classify_image_file(const char *path, file_info_t *info);


#endif
