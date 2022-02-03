#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

# Implements filter to automatically generate documentation of
# the interface to squirrel.

BEGIN {
	within_doxygen_comment = 0
	within_class = ""
	param_count = 0
	returns = "void"
	indent = ""
	within_apidoc = 0
	mask = ""
	ai_only = 0
}

# match beginning of SQAPI_DOC block
/^#ifdef.*SQAPI_DOC/ {
	within_sqapi_doc = 1
}

# match end of SQAPI_DOC block
/^#(endif|else)/ {
	if (within_sqapi_doc == 1) {
		within_sqapi_doc = 0
		param_count = 0
		delete params
		delete ptypes
		mask = ""
		returns = "void"
	}
}

# ignore preprocessor directives
/^#/ {
	next
}

/ingroup.*ai_only/ {
	ai_only = 1
}

function split_params(string)
{
	count = split(string, types, / *, */)
	for(i=1; i<=count;i++) {
		ptypes[i] = types[i];
	}
}

/@typemask/ {
	match($0, "@typemask *(.*)\\((.*)\\)", data)
	returns = data[1]
	split_params(data[2])
	next
}

# match beginning of doxgen comment block
/\/\*\*/ {
	within_doxygen_comment = 1
}

# print everything in a doxygen comment block
# and everything in a SQAPI_DOC block
{
	if (within_doxygen_comment==1) {
		print gensub( /^[[:space:]]*([ \\/])\\*/, indent "\\1", 1)
	}
	else if (within_sqapi_doc == 1) {
		print gensub( /^[[:space:]]*(.*)/, indent "\\1", 1)
	}
}

# match end of doxygen block
/\*\// {
	within_doxygen_comment = 0
}

# print doxygen brief commands, also //@
/\/\/[\/@]/ {
	if (within_doxygen_comment!=1  &&  within_sqapi_doc != 1) {
		print gensub( /^[[:space:]]*(\\.*)/, indent "\\1", 1)
	}
}

# beginning of class definition
# begin_class("factory_x", "extend_get");
/(begin|create)_.*class/ {
	# ignore class name inside error messages as in 'dbg->error("create_class", "failed ..")'
	if ( /dbg->(message|warning|error|fatal)/ ) {
		next
	}
	# class with parent class
	if ( /_class[^"]*"([^"]*)"[^"]*"([^"]*)".*/ ) {
		match($0, /_class[^"]*"([^"]*)"[^"]*"([^"]*)".*/, data)
		if (mode == "sq")
			print "class " data[1] " extends " data[2] " {"
		else {
			print "class " data[1] " : public " data[2] " {"
			print "public:"
		}
	}
	# class without parent class
	else if ( /_class[^"]*"([^"]*)"/ ) {
		match($0, /_class[^"]*"([^"]*)"/, data)
		if (mode == "sq")
			print "class " data[1] " {"
		else {
			print "class " data[1] " {"
			print "public:"
		}
	}
	within_class = data[1]
	indent = "\t"
	param_count = 0
	delete params
	delete ptypes
}

/end_.*class/ {
	if (mode == "sq")
		print "}"
	else
		print "};"
	within_class = ""
	indent = ""
}

# beginning of enum constant definition
/begin_enum/ {
	match($0, /^[[:space:]]*begin_enum[^"]*"([^"]*)"/, data)
	if (mode == "sq") {
		#print "class " data[1] " {"
	}
	else {
		print "enum " data[1] " {"
	}
	indent = "\t"
}

# end of enum
/end_enum/ {
	if (mode == "sq") {
		#print "class " data[1] " {"
	}
	else {
		print "};"
	}
	indent = ""
}
# @param names
/@param/ {
	if (within_doxygen_comment==1) {
		param_count++
		params[param_count] = $3
	}
}

# now the actual methods
# first string enclosed by ".." is method name
/register_function/  ||  /register_method/ ||  /register_local_method/{
	match($0, /"([^"]*)"/, data)
	method = data[1]
	if (ai_only == 1) {
		suffix = "ai"
	}
	else {
		suffix = "scenario"
	}
	# check for param types
	if (ai_only  &&   ((within_class "::" method) in export_types_ai)) {
		mask = export_types_ai[(within_class "::" method)]
	}
	else if ((within_class "::" method) in export_types_scenario) {
		mask = export_types_scenario[(within_class "::" method)]
	}
	if (mask != "") {
		match(mask, " *(.*)\\((.*)\\)", data)
		returns = data[1]
		split_params(data[2])
		for (t in ptypes) {
			if (!(t in params)) {
				params[t]=""
				param_count++
			}
		}
	}

	# build the output
	fname = ""
	if ( /STATIC/ ) {
		fname = fname "static "
	}
	if (mode == "sq") {
		if (method != "constructor") {
			fname = fname "function "
		}
		fname = fname method "("
	}
	else {
		if (method ~ /^_/) {
			print "private:"
		}
		if (method == "constructor") {
			fname = fname within_class "("
		}
		else {
			fname = fname returns " " method "("
		}
	}
	for (param = 1; param <= param_count; param++) {
		if (mode != "sq") {
			if (!(param in ptypes)) ptypes[param] = "any_x"
			fname = fname ptypes[param]
		}
		if (params[param] != "") {
			fname = fname " " params[param]
		}
		if (param < param_count) fname = fname ", "
	}
	fname = fname  ");"
	print indent fname

	if ( (mode == "cpp")  &&  (method ~ /^_/)) {
		print "public:"
	}

	param_count = 0
	delete params
	delete ptypes
	mask = ""
	returns = "void"
	ai_only = 0
}

# enum constants
/enum_slot/ {
	match($0, /"([^"]*)"/, data)
	name = data[1]
	print indent name ","
}
