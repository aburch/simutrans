include("hm_lib/hm_global")

// valid only for wt_rail
class hm_wayobj_tl extends hm_base_tl {
  desc_name = null
  start     = null
  ziel      = null
  wtype     = null
  overhead  = null

  constructor(d_name, s, z, wt = null, oh = null) {
    desc_name = d_name
    start     = coord3d(s[0],s[1],s[2])
    ziel      = coord3d(z[0],z[1],z[2])
    wtype     = wt
    overhead  = oh
    hm_commands.append(this)
  }

  // returns [error_message, desc]
  function _get_desc(){
    if(desc_name.slice(0,2)=="?f") {
      local key_str = "c" + desc_name.slice(2)
      local d = hm_found_desc.get(key_str)
      if(d==null) {
        local message = format(translate("Wayobj key %s is not defined."), desc_name.slice(2))
        return [message, null]
      } else if(d[0]==null) {
        local message = format(translate("No wayobj was detected between %s."), hm_found_desc.get_pos_str(key_str))
        return [message, null]
      }
      return [null, d[0]]
    }
    else if(desc_name.slice(0,2)=="?s") {
      local idx = desc_name.slice(2).tointeger()
      local d = hm_wayobj_selector().get_desc(idx)
      if(d==null) {
        local message = format(translate("Selected wayobj %s is not available."), desc_name.slice(2))
        return [message, null]
      }
      return [null, d]
    }
    else {
      local d = hm_get_wayobjs_desc(desc_name, wtype, overhead)
      //gui.add_message_at(player, "_get_desc - hm_get_way_desc() " + d, world.get_time())
      if(d==null) {
        local message = format(translate("Wayobj %s (%s) is not found!"), translate(desc_name), desc_name)
        return [message, null]
      }
      return [null, d]
    }
  }

  function exec(player, origin) {
    local dr = _get_desc()
    if(dr[0]!=null) {
      return dr[0] // there was a error in obtaining the desc
    }
    local desc = dr[1]
    local err = command_x.build_wayobj(player, origin+start, origin+ziel, desc)
    if(err!=null) {
      //calc_route() failed to find a path.
      local message = format(translate("Wayobj building path from ($s) to (%s) is not found!"), (origin+start).tostring(), (origin+ziel).tostring())
      return message
    } else {
      return null
    }
  }
}
