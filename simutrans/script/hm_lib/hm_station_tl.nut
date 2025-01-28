include("hm_lib/hm_global")

class hm_station_tl extends hm_base_tl {
  desc      = null
  desc_name = null
  pos       = null
  wtype     = 0
  rotation  = 15
  building  = 34 // generic stop

  constructor(sta_name, p, wt = 0, r = 15, b = 34) {
    desc_name = sta_name
    pos       = coord3d(p[0],p[1],p[2])
    wtype     = wt
    rotation  = r
    hm_commands.append(this)
    building  = b
  }

  // returns [error_message, desc]
  function _get_desc(){
    if(desc_name.slice(0,2)=="?f") {
      local key_str = "p" + desc_name.slice(2)
      local d = hm_found_desc.get(key_str)
      if(d==null) {
        local message = format(translate("Station key %s is not defined."), desc_name.slice(2))
        return [message, null]
      } else if(d[0]==null) {
        local message = format(translate("No station was detected between %s."), hm_found_desc.get_pos_str(key_str))
        return [message, null]
      }
      return [null, d[0]]
    } else if(desc_name.slice(0,2)=="?s") {
      local idx = desc_name.slice(2).tointeger()
      local d = hm_station_selector().get_desc(idx)
      if(d==null) {
        local message = format(translate("Selected station %s is not available."), desc_name.slice(2))
        return [message, null]
      }
      return [null, d]
    } else {
      local d = hm_get_building_desc(desc_name, wtype, building)
      if(d==null) {
        local message = format(translate("Station %s (%s) is not found!"), translate(desc_name), desc_name)
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
    return command_x.build_station(player, origin+pos, desc, rotation);
  }
}
