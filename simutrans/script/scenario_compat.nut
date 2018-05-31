/**
 * File with compatibility functions to run scripts written for earlier versions of the api.
 */
function compat(version)
{
	if (version == "*") {
		return; // wants bleeding edge stuff
	}

	local f = type(version)=="string" ? version.tofloat() : 112.3;

	// default version
	if (f <= 112.3) {
		f = 112.3
	}

	// sorted in descending order w.r.t. api version
	local all_versions = [
		{ v = 112.3, f = compat_112_3},
	]

	foreach(vt in all_versions) {
		if (f - 0.01 <= vt.v) {
			vt.f.call(this)
		}
	}
}


function compat_112_3()
{
	print("Compatibility mode for version 112.3 in effect")

	// convoy_list_x() deprecated, set default value to iterate through global list
	convoy_list_x.use_world <- 1
}
