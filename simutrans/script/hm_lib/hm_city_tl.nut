include("hm_lib/hm_global")


class hm_city_tl extends hm_base_tl {
  population = 0
  desc_name  = 0
  pos        = null
  rotation   = 0

  constructor(pop, dn, p, r) {
    population = pop
    desc_name  = dn
    pos        = coord3d(p[0],p[1],0)
    rotation   = r
    hm_commands.append(this)
  }

  function exec(player, origin) {
    local tp = origin + pos
    local tile = square_x(tp.x,tp.y).get_ground_tile()
    if(tile) {
      return command_x(tool_add_city).work(player, tile, format("%d,%s,%d", population, desc_name, rotation))
    }
    return "No suitable ground!"
  }
}


class hm_city_set_population_tl extends hm_base_tl {
  pos         = null
  population  = 0

  constructor(pop, p) {
    pos        = coord(p[0],p[1])
    population = pop
    hm_commands.append(this)
  }

  function exec(player, origin) {
    local tp = origin+pos
    local tile = square_x(tp.x,tp.y).get_ground_tile()
    if(tile) {
      return command_x(tool_change_city_size).work(player, tile, format("%d", population))
    }
    return "No suitable ground!"
  }
}


class hm_set_owner_tl extends hm_base_tl {
  pos     = null
  ownerid = 0
  has_z   = false

  constructor(o, p) {
    pos     = coord3d(p[0],p[1],0)
    if(p.len()>2) {
      pos.z = p[2]
      has_z = true
    }
    ownerid = o
    hm_commands.append(this)
  }

  function exec(player, origin) {
    local tp = origin+pos
    local tile = square_x(tp.x, tp.y).get_tile_at_height(tp.z)
    if(tile == null  &&  !has_z) {
      tile = square_x(tp.x, tp.y).get_ground_tile()
    }
    if(tile) {
      local pl = player_x(ownerid)
      return command_x(tool_set_owner).work(player, tile, format("%d,0",ownerid))
//    foreach(obj in tile.get_objects()) {
//      // owner to0l
//    }
    }
    if(tile.has_ways()  &&  ownerid == 15) {
      // no way to convert to pavement roads right now
    }
    return null
  }
}
