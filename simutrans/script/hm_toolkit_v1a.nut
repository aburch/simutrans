
class hm_slope {
  N1 = 4
  N2 = 8
  S1 = 36
  S2 = 72
  E1 = 28
  E2 = 56
  W1 = 12
  W2 = 24
  UP = 82
  //UP2 = 92
  DOWN = 83
  //DN2 = 93
}

local hm_commands = [] // array to store the commands
local hm_all_waytypes = [wt_road,wt_rail,wt_water,wt_monorail,wt_maglev,wt_tram,wt_narrowgauge,wt_air,wt_power]

class hm_base_tl {
  function exec(player, origin) {
    return null
  }
}

class hm_slope_tl extends hm_base_tl {
  slope = 0
  pos = null
  constructor(slp, p) {
    slope = slp
    pos = coord3d(p[0],p[1],p[2])
    hm_commands.append(this)
  }
  function exec(player, origin) {
    // check if the designated pos refers a ground tile.
    local tp = origin+pos
    local tile = square_x(tp.x, tp.y).get_ground_tile()
    if(tile.z!=tp.z || !tile.is_ground()) {
      return "Tile " + tp.tostring() + " is not a valid ground!"
    }
    return command_x.set_slope(player, tp, slope);
  }
}

class hm_way_tl extends hm_base_tl {
  desc = null
  desc_name = null
  start = null
  ziel = null
  constructor(d_name, s, z) {
    desc_name = d_name
    foreach(wt in hm_all_waytypes) {
      foreach (st in [st_flat, st_elevated, st_tram]) {
        foreach (w in way_desc_x.get_available_ways(wt, st)) {
          if(w.get_name()==desc_name) {
            desc = w
            break
          }
        }
      }
      if(desc!=null) {
        break
      }
    }
    start = coord3d(s[0],s[1],s[2])
    ziel = coord3d(z[0],z[1],z[2])
    hm_commands.append(this)
  }
  function exec(player, origin) {
    if (desc==null) {
      return "Way " + desc_name + " is not found!"
    }
    return command_x.build_way(player, origin+start, origin+ziel, desc, true)
  }
}

// valid only for wt_rail
class hm_wayobj_tl extends hm_base_tl {
  desc_name = null
  start = null
  ziel = null
  constructor(d_name, s, z) {
    desc_name = d_name
    start = coord3d(s[0],s[1],s[2])
    ziel = coord3d(z[0],z[1],z[2])
    hm_commands.append(this)
  }
  function exec(player, origin) {
    local tl = command_x(tool_build_wayobj)
    return tl.work(player, origin+start, origin+ziel, desc_name)
  }
}

class hm_sign_tl extends hm_base_tl {
  desc = null
  desc_name = null
  pos = null
  num_exec = 1
  constructor(sign_name, num, p) {
    desc_name = sign_name
    pos = coord3d(p[0],p[1],p[2])
    num_exec = num
    // find descriptor
    foreach(wt in hm_all_waytypes) {
      foreach (s in sign_desc_x.get_available_signs(wt)) {
        if(s.get_name()==desc_name) {
          desc = s
          break
        }
      }
      if(desc!=null) {
        break
      }
    }
    hm_commands.append(this)
  }
  function exec(player, origin) {
    if (desc==null) {
      return "Sign " + desc_name + " is not found!"
    }
    // check if the way exists
    local s_pos = origin + pos
    local way = tile_x(s_pos.x, s_pos.y, s_pos.z).find_object(mo_way)
    if(way==null || !dir.is_twoway(way.get_dirs()) || way.get_waytype()!=desc.get_waytype()) {
      // we silently ignore single way tiles for signals as building will fail on them
      //return "Not suitable way for sign at " + s_pos.tostring()
    }
    else {
      return command_x.build_sign_at(player, s_pos, desc)
    }
  }
}

class hm_station_tl extends hm_base_tl {
  desc = null
  desc_name = null
  pos = null
  constructor(sta_name, p) {
    desc_name = sta_name
    pos = coord3d(p[0],p[1],p[2])
    // find descriptor
    foreach (d in building_desc_x.get_building_list(building_desc_x.station)) {
      if(d.get_name()==desc_name) {
        desc = d
        break
      }
    }
    hm_commands.append(this)
  }
  function exec(player, origin) {
    if (desc==null) {
      return "Station " + desc_name + " is not found!"
    }
    return command_x.build_station(player, origin+pos, desc);
  }
}

class hm_wayremove_tl extends hm_base_tl {
  start = null
  ziel = null
  constructor(s, z) {
    start = coord3d(s[0],s[1],s[2])
    ziel = coord3d(z[0],z[1],z[2])
    hm_commands.append(this)
  }
  function exec(player, origin) {
    // determine waytype
    local s_pos = origin + start
    local way = tile_x(s_pos.x, s_pos.y, s_pos.z).find_object(mo_way)
    if(way==null) {
      return "No ways were found at " + s_pos.tostring()
    }
    local tl = command_x(tool_remove_way)
    return tl.work(player, origin+start, origin+ziel, way.get_waytype().tostring())
  }
}

class hm_remove_tl extends hm_base_tl {
  pos = null
  constructor(p) {
    pos = coord3d(p[0],p[1],p[2])
    hm_commands.append(this)
  }
  function exec(player, origin) {
    local tl = command_x(tool_remover)
    return tl.work(player, origin+pos)
  }
}

function init(player) {
  hm_build()
  return true
}

function work(player, pos) {
  foreach (cmd in hm_commands) {
    local err = cmd.exec(player, pos)
    if(err!=null && err.len()>0) {
      return err
    }
  }
  return null
}

function exit(pl)
{
	return true
}
