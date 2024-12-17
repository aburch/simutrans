include("hm_lib/hm_global")

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
      local message = format(translate("No ways were found at %s."), coord3d_to_string(s_pos))
      return [message, null]
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
