/*
 *
 *  searchfolder.cpp
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */

#include <string.h>
#include <sys/stat.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <dirent.h>
#else
#include <io.h>
#endif

#include "../simdebug.h"
#include "searchfolder.h"

#ifdef _MSC_VER
#define STRICMP stricmp
#else
#define STRICMP strcasecmp
#endif

 /*
 *  member function:
 *      searchfolder_t::search()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Endet filepath mit einem slash so werden alle Files im diesem
 *	Verzeichnis mit der angegebene Extension gesucht.
 *      Endet filepath nicht mit einem slash und enthält keinen Punkt
 *	nach dem letzten Slash. So wird er um die Extension erweitert
 *	und gesucht.
 *	Ansonsten wird direkt nach filepath gesucht.
 *
 *	Keine Wildcards, bitte!
 *
 *  Return type:
 *      int	    Anzahl gefundener Dateien.
 *
 *  Argumente:
 *      cstring_t filepath
 *      cstring_t ext
 */
int searchfolder_t::search(const char *filepath, const char *extension)
{
    cstring_t path;
    cstring_t name;
    cstring_t lookfor;
    cstring_t ext;

    files.clear();

    path = filepath;
    if(path.right(1) == "/") {
	// Look for a directory
	name = "*";
	ext = cstring_t(".") + extension;
    } else {
	int slash = path.find_back('/');
	int dot = path.find_back('.');

	if(dot == -1 || dot < slash) {
	    // Look for a file with default extension
	    name = path.mid(slash + 1);
	    path = path.left(slash + 1);
	    ext = cstring_t(".") + extension;
	}
	else {
	    // Look for a file with own extension
	    ext = path.mid(dot);
	    name = path.mid(slash + 1, dot - slash - 1);
	    path = path.left(slash + 1);
	}
    }
#ifdef _MSC_VER
    lookfor = path + name + ext;
    struct _finddata_t entry;
    long hfind = _findfirst(lookfor.chars(), &entry);

    if(hfind != -1) {
	lookfor = ext;
	do {
	    if(stricmp(entry.name + strlen(entry.name) - lookfor.len(), lookfor.chars()) == 0) {
		files.append(path + "/" + entry.name);
	    }
	} while(_findnext(hfind, &entry) == 0 );
    }
#else
    lookfor = path + ".";

    DIR *dir = opendir(lookfor.chars());
    struct  dirent  *entry;

    if(dir != NULL) {
    	//printf("....Search folder %s\n", lookfor.chars());
	lookfor = name = "*" ? ext : name + ext;
    	//printf("....must match %s\n", lookfor.chars());
	while((entry = readdir(dir)) != NULL) {
	    if(entry->d_name[0]!='.' || (entry->d_name[1]!='.' && entry->d_name[1]!=0)) {
		if(strcasecmp(entry->d_name + strlen(entry->d_name) - lookfor.len(), lookfor.chars()) == 0) {
    		    //printf("....%s matches\n", entry->d_name);
		    files.append(path + "/" + entry->d_name);
		}
	    	//else printf("....%s does not match\n", entry->d_name);
	    }
	}
	closedir(dir);
    }
#endif
    return files.count();
}

cstring_t searchfolder_t::complete(const char *filepath, const char *extension)
{
    cstring_t path = filepath;

    if(path.right(1) != "/") {
	int slash = path.find_back('/');
	int dot = path.find_back('.');

	if(dot == -1 || dot < slash) {
	    return path + "." + extension;
	}
    }
    return path;
}

/**
 *  member function:
 *      searchfolder_t::at()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Liefert das i-te Ergebnis der letzten Suche.
 *
 *  Return type:
 *      cstring_t
 *
 *  Argumente:
 *      int i
 */
const cstring_t &searchfolder_t::at(unsigned int i)
{
    static cstring_t nix;

    return i < files.count() ? files.at(i) : nix;
}
