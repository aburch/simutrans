/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UTILS_SEARCHFOLDER_H
#define UTILS_SEARCHFOLDER_H


#include <string>
#include "../tpl/vector_tpl.h"


/**
 * Searches a disk folder for files matching certain restrictions.
 */
class searchfolder_t
{
public:
	enum search_flags_t : uint8
	{
		SF_NONE         = 0,
		SF_ONLYDIRS     = 1 << 0, ///< Extra restriction, will only consider directory entries. If not specified, will search both file and directory entries.
		SF_PREPEND_PATH = 1 << 1, ///< Will force prepending the base path to the output on each entry.
		SF_NOADDONS     = 1 << 2  ///< If set, subdrectories named "addons" will not be searched.
	};

public:
	~searchfolder_t();

	/**
	 * Executes the requested file search. It will look in subdirectories recursively when a max_depth is given.
	 * @param filepath Base path to search in.
	 * @param extension Extension of files to search for. Input a empty string to not enforce the restriction.
	 * @param only_directories Extra restriction, will only consider directory entries.
	 * @param prepend_path Will force prepending the base path to the output on each entry.
	 * @param max_depth Maximum depth of recursive search (0=search only the directory given by @p filepath).
	 * @returns Number of files that matched the search parameters.
	 */
	int search(const std::string &filepath, const std::string &extension, search_flags_t flags = SF_PREPEND_PATH, const int max_depth = 0);

	/**
	 * Appends the extension to the file name if it's not a directory
	 * @param filepath Path to the file
	 * @param extension File name extension to append if the conditions are meet
	 */
	static std::string complete(const std::string &filepath, const std::string &extension);

	/**
	 * Function results will be accessible only in a const iterator fashion.
	 */
	typedef vector_tpl<char*>::const_iterator const_iterator;
	/**
	 * Gets the first entry iterator.
	 */
	const_iterator begin() const { return files.begin(); }
	/**
	 * Gets the last entry iterator.
	 */
	const_iterator end()   const { return files.end();   }

private:
	/**
	 * Make the preparations for the search and then call search_path to do the actual search.
	 * @param filepath Base path to search in. (This path can have either / or \ as deliminator on windows.
	 * @param extension Extension of files to search for. Input a empty string to not enforce the restriction.
	 * @param only_directories Extra restriction, will only consider directory entries.
	 * @param prepend_path Will force prepending the base path to the output on each entry.
	 * @param max_depth Maximum depth of recursive search.
	 */
	int prepare_search(const std::string &filepath, const std::string &extension, search_flags_t search_flags, const int max_depth = 0);

	/**
	 * Real implementation of the search. It will call itself recursively to search in subdirectories when a max_depth is given.
	 * @param path Path to search in. (This path can have either / or \ as deliminator on windows.
	 * @param name Name of file to search. It will be "*" if not searching for an specific file.
	 * @param ext Extension of files to search for.
	 * @param only_directories Extra restriction, will only consider directory entries.
	 * @param prepend_path Will force prepending the base path to the output on each entry.
	 * @param max_depth Maximum depth of recursive search.
	 */
	void search_path(const std::string path, const std::string name, const std::string ext, search_flags_t search_flags, const int max_depth = 0 );

	/**
	 * We store the result of the search on this list
	 */
	vector_tpl<char*> files;

	/**
	 * Adds one entry to the list.
	 * @param path Qualified path to the directory of the entry.
	 * @param entry Filename of the file to add.
	 * @param prepend Add the full path to the file or just the file name.
	 */
	void add_entry(const std::string &path, const char *entry, const bool prepend );

	/**
	 * Checks if @p fn matches @p pattern, case insensitive.
	 * @param pattern may start with an * as a wildcard that matches 0 or more characters.
	 */
	bool filename_matches_pattern(const char *fn, const char *pattern);

	/**
	 * Clears the search results.
	 */
	void clear_list();
};


ENUM_BITSET(searchfolder_t::search_flags_t);


#endif
