/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "../simdebug.h"
#include "../simgraph.h"
#include "koord.h"
#include "tabfile.h"


bool tabfile_t::open(const char *filename)
{
	close();
	file = fopen(filename, "r");
	return file != NULL;
}



void tabfile_t::close()
{
	if(file) {
		fclose(file);
		file  = NULL;
	}
}


const char *tabfileobj_t::get(const char *key) const
{
	const char *result = objinfo.get(key);
	return result ? result : "";
}


/**
 * Get the string value for a key - key must be lowercase
 * @return def if key isn't found, value otherwise
 * @author Hj. Malthaner
 */
const char *tabfileobj_t::get_string(const char *key, const char * def)
{
	const char *result = objinfo.get(key);

	return result ? result : def;
}



bool tabfileobj_t::put(const char *key, const char *value)
{
	if(objinfo.get(key)) {
		return false;
	}
	objinfo.put(strdup(key), strdup(value));
	return true;
}



void tabfileobj_t::clear()
{
	FOR(stringhashtable_tpl<char const*>, const& i, objinfo) {
		free(const_cast<char*>(i.key));
		free(const_cast<char*>(i.value));
	}
	objinfo.clear();
}

const koord &tabfileobj_t::get_koord(const char *key, koord def)
{
	static koord ret;
	const char *value = get(key);
	const char *tmp;

	ret = def;

	if(!value || !*value) {
		return ret;
	}
	// 2. Wert bestimmen
	for(tmp = value; *tmp != ','; tmp++) {
		if(!*tmp) {
			return ret;
		}
	}
	ret.x = atoi(value);
	ret.y = atoi(tmp + 1);
	return ret;
}

uint8 tabfileobj_t::get_color(const char *key, uint8 def)
{
#ifdef MAKEOBJ
	// don't have access to colour conversion code so just return default
	return def;
#else
	const char *value = get(key);

	if(!value || !*value) {
		return def;
	}
	else {
		// skip spaces/tabs
		while ( *value>0  &&  *value<=32  ) {
			value ++;
		}
		if(  *value=='#'  ) {
			uint32 rgb = strtoul( value+1, NULL, 16 ) & 0XFFFFFFul;
			return display_get_index_from_rgb( rgb>>16, (rgb>>8)&0xFF, rgb&0xFF );
		}
		else {
			// this inputs also hex correct
			return (uint8)strtol( value, NULL, 0 );
		}
	}
#endif
}

int tabfileobj_t::get_int(const char *key, int def)
{
	const char *value = get(key);

	if(!value || !*value) {
		return def;
	}
	else {
		// skip spaces/tabs
		while ( *value>0  &&  *value<=32  ) {
			value ++;
		}
		// this inputs also hex correct
		return strtol( value, NULL, 0 );
	}
}


sint64 atosint64(const char* a)
{
	return (sint64)(atof(a)+0.5);
}


sint64 tabfileobj_t::get_int64(const char *key, sint64 def)
{
	const char *value = get(key);

	if(!value || !*value) {
		return def;
	}
	else {
		return atosint64(value);
	}
}

int *tabfileobj_t::get_ints(const char *key)
{
	const char *value = get(key);
	const char *tmp;
	int         count = 1;
	int         *result;

	if(!value || !*value) {
		result = new int[1];
		result[0] = 0;
		return result;
	}
	// Anzahl bestimmen
	for(tmp = value; *tmp; tmp++) {
		if(*tmp == ',') {
			count++;
		}
	}
	// Ergebnisvektor erstellen und füllen
	result = new int[count + 1];

	result[0] = count;
	count = 1;
	result[count++] = strtol( value, NULL, 0 );
	for(tmp = value; *tmp; tmp++) {
		if(*tmp == ',') {
			// skip spaces/tabs
			do {
				tmp ++;
			} while ( *tmp>0  &&  *tmp<=32  );
			// this inputs also hex correct
			result[count++] = strtol( tmp, NULL, 0 );
		}
	}
	return result;
}


sint64 *tabfileobj_t::get_sint64s(const char *key)
{
	const char *value = get(key);
	const char *tmp;
	int         count = 1;
	sint64         *result;

	if(!value || !*value) {
		result = new sint64[1];
		result[0] = 0;
		return result;
	}
	// Anzahl bestimmen
	for(tmp = value; *tmp; tmp++) {
		if(*tmp == ',') {
			count++;
		}
	}
	// Ergebnisvektor erstellen und füllen
	result = new sint64[count + 1];

	result[0] = count;
	count = 1;
	result[count++] = atosint64(value);
	for(tmp = value; *tmp; tmp++) {
		if(*tmp == ',') {
			result[count++] = atosint64(tmp + 1);
		}
	}
	return result;
}


bool tabfile_t::read(tabfileobj_t &objinfo)
{
	bool lines = false;
	char line[4096];
	objinfo.clear();

	do {
		while(read_line(line, sizeof(line)) && *line != '-') {
			char *delim = strchr(line, '=');

			if(delim) {
				*delim++ = '\0';
				format_key(line);
				objinfo.put(line, delim);
				lines = true;
			}
		}
	} while(!lines && !feof(file)); // skip empty objects

	return lines;
}



bool tabfile_t::read_line(char *s, int size)
{
	char *r;
	size_t l;

	do {
		r = fgets(s, size, file);
	} while(r != NULL  &&  (*s == '#' || *s == ' ')  );

	if(r) {
		l = strlen(r);
		while(  l  &&  (r[l-1] == '\n' || r[l-1] == '\r')  ) {
			r[--l] = '\0';
		}
	}
	return r != NULL;
}



void tabfile_t::format_key(char *key)
{
	char *s = key + strlen(key);
	char *t;

	// trim right
	while(s > key && s[-1] == ' ') {
		*--s = '\0';
	}
	// make lowercase
	for(s = key; *s; s++) {
		*s = tolower(*s);
	}
	// skip spaces inside []
	for(s = t = key; *s; s++) {
		if(*s == '[') {
			*t++ = *s++;

			while(*s && *s != ']') {
				if(*s == ' ') {
					s++;
				}
				else {
					*t++ = *s++;
				}
			}
			s--;
		}
		else {
			*t++ = *s;
		}
	}
	*t = '\0';
}



void tabfile_t::format_value(char *value)
{
	size_t len = strlen(value);

	// trim right
	while(len && value[len - 1] == ' ') {
		value[--len] = '\0';
	}
	// trim left
	if(*value == ' ') {
		char *from;
		for(from = value; *from == ' '; from++) {}
		while(*value) {
			*value++ = *from++;
		}
	}
}
