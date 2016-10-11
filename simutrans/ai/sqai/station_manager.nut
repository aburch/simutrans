class freight_station_t
{
	factory  = null // factory_x
	depot    = null // tile_x
	road_unload = null // tile_x

	constructor(f)
	{
		factory = f
	}

	function _save()
	{
		return ::saveinstance("freight_station_t", this)
	}
}



class freight_station_manager_t extends node_t
{
	freight_station_list = null

	constructor()
	{
		base.constructor("freight_station_manager_t")
		freight_station_list = {}
		::station_manager = this
	}

	/// Generate unique key from link data
	static function key(factory)
	{
		return (factory.get_name() + coord_to_string(factory)).toalnum()
	}

	/**
	 * Access freight_station_t data, create node if not existent.
	 */
	function access_freight_station(factory)
	{
		local k = key(factory)
		local res
		try {
			res = freight_station_list[k]
		}
		catch(ev) {
			local fs = freight_station_t(factory)
			freight_station_list[k] <- fs
			res = fs
		}
		return res
	}
}
