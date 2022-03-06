/**
 * Helper static functions to find places for stations, depots, etc.
 */

class finder {

  static function get_tiles_near_factory(factory)
  {
    local cov = settings.get_station_coverage()
    local area = []

    // generate a list of tiles that will reach the factory
    local ftiles = factory.get_tile_list()
    foreach (c in ftiles) {
      for(local dx = -cov; dx <= cov; dx++) {
        for(local dy = -cov; dy <= cov; dy++) {
          if (dx==0 && dy==0) continue;

          local x = c.x+dx
          local y = c.y+dy

          if (x>=0 && y>=0) area.append( (x << 16) + y );
        }
      }
    }
    // sort
    sleep()
    area.sort()
    return area
  }

  static function get_tiles_near_halt(halt)
  {
    local cov = 1
    local area = []

    // generate a list of tiles that will reach the halt for combined
    local ftiles = halt.get_tile_list()
    foreach (c in ftiles) {
      for(local dx = -cov; dx <= cov; dx++) {
        for(local dy = -cov; dy <= cov; dy++) {
          if (dx==0 && dy==0) continue;

          local x = c.x+dx
          local y = c.y+dy

          if (x>=0 && y>=0) area.append( (x << 16) + y );
        }
      }
    }
    // sort
    sleep()
    area.sort()
    return area
  }

  static function find_empty_place(area, target)
  {
    // find place closest to target
    local tx = target.x
    local ty = target.y

    local best = null
    local dist = 10000
    // check for flat and empty ground
    for(local i = 0; i<area.len(); i++) {

      local h = area[i]
      if (i>0  &&  h == area[i-1]) continue;

      local x = h >> 16
      local y = h & 0xffff

      if (world.is_coord_valid({x=x,y=y})) {
        local tile = square_x(x, y).get_ground_tile()

        if (tile.is_empty()  &&  tile.get_slope()==0) {
          local d = abs(x - tx) + abs(y - ty)
          if (d < dist) {
            dist = d
            best = tile
          }
        }
      }
    }
    return best
  }


  static function _find_places(area, test /* function */)
  {
    local list = []
    // check for flat and empty ground
    for(local i = 0; i<area.len(); i++) {

      local h = area[i]
      if (i>0  &&  h == area[i-1]) continue;

      local x = h >> 16
      local y = h & 0xffff

      if (world.is_coord_valid({x=x,y=y})) {
        local tile = square_x(x, y).get_ground_tile()

        if (test(tile)) {
          list.append(tile)
        }
      }
    }
    return list.len() > 0 ?  list : []
  }

  static function find_empty_places(area)
  {
    return _find_places(area, _tile_empty)
  }

  static function _tile_empty(tile)
  {
    return tile.is_empty()  &&  tile.get_slope()==0
  }

  static function _tile_empty_or_field(tile)
  {
    return tile.get_slope()==0  &&  (tile.is_empty()  ||  tile.find_object(mo_field))
  }

  static function find_water_places(area)
  {
    return _find_places(area, _tile_water)
  }

  static function _tile_water(tile)
  {
    return tile.is_water()  &&  (tile.find_object(mo_building)==null)  &&  (tile.find_object(mo_depot_water)==null)
  }

  static function _tile_water_way(tile)
  {
    if (tile.is_water()) {
      return true // (tile.find_object(mo_building)==null)  &&  (tile.find_object(mo_depot_water)==null)
    }
    else {
      foreach(obj in tile.get_objects()) {
        try {
          if (obj.get_type() != mo_way) continue;

          if (obj.get_waytype() == wt_water) {
            return obj.get_desc().get_topspeed() > 5
          }
        }
        catch(ev) {
          // object gone, most likely vehicle that moved on
        }
      }
    }
    return false
  }

  static function find_station_place(factory, target, unload = false)
  {
    if (unload) {
      // try unload station from station manager
      local res = ::station_manager.access_freight_station(factory).road_unload
      if (res) {
        return [res]
      }
    }
    local can_delete_fields = factory.get_field_count() > factory.get_min_field_count()

    local area = get_tiles_near_factory(factory)

    if (can_delete_fields) {
      return _find_places(area, _tile_empty_or_field);
    }
    else {
      return find_empty_places(area)
    }
  }

  /**
   * Can harbour of length @p len placed at @p pos (land tile!) in direction @p d.
   */
  static function check_harbour_place(pos, len, d /* direction */)
  {
    local from = pos
    for(local i = 0; i<len; i++) {
      local to = from.get_neighbour(i>0 ? wt_water : wt_all, d)
      if (to  &&  _tile_water(to)  &&  to.can_remove_all_objects(our_player)==null) {
        from = to
      }
      else {
        return false
      }
    }
    return true
  }

}
