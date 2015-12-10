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
		{ v = 120.1, f = compat_120_1},
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

function compat_120_1()
{
	print("Compatibility mode for version 120.1 in effect")

	// 120.1 broke player_x::is_active (r7603)
	player_x.is_active <- function() {
		try {
			this.get_name() // will throw if no player with this number exists
			return true
		}
		catch(ev) {
			return false
		}
	}
}
