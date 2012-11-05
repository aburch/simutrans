/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 *
 *  Functionality:
 *      Searches a disk folder for files matching certain restrictions.
 */
#ifndef __SEARCHFOLDER_H
#define __SEARCHFOLDER_H


#include <string>
#include "../tpl/vector_tpl.h"


/*!
 * \author Volker Meyer
 * \author Markohs
 */
class searchfolder_t {
public:
	~searchfolder_t();
/*!
 * Executes the requested file search
 * \param filepath Base path to search in.
 * \param extension Extension of files to search for. Input a empty string to not enforce the restriction.
 * \param only_directories Extra restriction, will only consider directory entries.
 * \param prepend_path Will force prepending the base path to the output on each entry.
 */
	int search(const std::string &filepath, const std::string &extension, const bool only_directories = false, const bool prepend_path = true);

	static std::string complete(const std::string &filepath, const std::string &extension);

/*!
 * Function results will be accesible only in a const interator fashion.
 */
	typedef vector_tpl<char*>::const_iterator const_iterator;
/*!
 * Gets the first entry iterator.
 */
	const_iterator begin() const { return files.begin(); }
/*!
 * Gets the last entry iterator.
 */
	const_iterator end()   const { return files.end();   }

private:
/*!
 * Real implementation of the search.
 */
	int search_path(const std::string &filepath, const std::string &extension, const bool only_directories = false, const bool prepend_path = true);
/*!
 * We store the result of the search on this list
 */
	vector_tpl<char*> files;
/*!
 * Adds one entry to the list.
 */
	void add_entry(const std::string &path, const char *entry, const bool prepend );
/*!
 * Clears the seach results.
 */
	void clear_list();
};

#endif
