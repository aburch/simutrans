/**
 * Contains helpers to save and load instances using strings.
 */

// functions to load/save an instance
function loadinstance(classname, table)
{
	local new_class    = this[classname]
	local new_instance = new_class.instance()
	foreach(key, val in table) {
		try {
			new_instance[key] = val
		}
		catch(ev) { }
	}
	if (classname == "factorysearcher_t" ) {
		factorysearcher = new_instance
	}
	else if (classname == "industry_manager_t" ) {
		industry_manager = new_instance
		industry_manager.repair_keys()
	}
	else if (classname == "freight_station_manager_t" ) {
		station_manager = new_instance
		station_manager.repair_keys()
	}
	else if (classname == "ship_connector_t") {
		new_instance.repair_keys()
	}

	return new_instance
}

function saveinstance(classname, instance)
{
	local t = {}
	local my_class = instance.getclass()
	foreach (key, val in my_class) {
		if (typeof(val) == "function") {
			continue
		}
		// only save slots that are not equal to default values
		if (val != instance[key]) {
			t[key] <- instance[key]
		}
	}
	return "::loadinstance(\"" + classname + "\", " + recursive_save(t, "\t\t\t", []) + ")"
}

// annotate all classes in the root table
function annotate_classes()
{
	foreach(key, val in getroottable())
	{
		if (typeof(val) == "class") {
			val.setattributes(null, {classname = key} )
		}
	}
}

// additional save functions for built-in types
factory_x._save <- function ()
{
	return "factory_x(" + x + ", " + y + ")"
}
line_x._save <- function ()
{
	return "line_x(" + id + ")"
}
halt_x._save <- function ()
{
	return "halt_x(" + id + ")"
}
convoy_x._save <- function ()
{
	return "convoy_x(" + id + ")"
}
tile_x._save <- function ()
{
	return "tile_x(" + x + ", " + y + ", " + z + ")"
}
depot_x._save <- function ()
{
	return "depot_x(" + x + ", " + y + ", " + z + ")"
}

// additional save functions for *_desc_x-types
function loaddesc(classname, name)
{
	local new_class    = this[classname]
	local new_instance = new_class.instance()
	try {
		new_instance.constructor(name)
	}
	catch(ev) {
		new_instance = name
	}
	return new_instance
}

obj_desc_x._save <- function ()
{
	local my_class = this.getclass()
	local attr = my_class.getattributes(null)
	if (attr  &&  ("classname" in attr)) {
		local name = attr.classname
		return "::loaddesc(\"" + name + "\", \"" + get_name() + "\")"
	}
	else {
		return "\"" + get_name() + "\""
	}
}

good_desc_x._save          <- obj_desc_x._save
obj_desc_time_x._save      <- obj_desc_x._save
building_desc_x._save      <- obj_desc_x._save
obj_desc_transport_x._save <- obj_desc_x._save
vehicle_desc_x._save       <- obj_desc_x._save
way_desc_x._save           <- obj_desc_x._save
tree_desc_x._save          <- obj_desc_x._save
