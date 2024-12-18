include("hm_lib/hm_global")
include("hm_lib/hm_functions")

class hm_sign_tl extends hm_base_tl {
  desc_name = null
  pos       = null
  num_exec  = 1
  wtype     = null

  constructor(sign_name, num, p, wt = null) {
    desc_name = sign_name
    pos       = coord3d(p[0],p[1],p[2])
    num_exec  = num
    wtype     = wt
    hm_commands.append(this)
  }

  // returns [error_message, desc]
  function _get_desc(){
    if(desc_name.slice(0,2)=="?f") {
      local key_str = "s" + desc_name.slice(2)
      local d = hm_found_desc.get(key_str)
      if(d==null) {
        local message = format(translate("Sign key %s is not defined."), desc_name.slice(2))
        return [message, null]
      } else if(d[0]==null) {
        local message = format(translate("No sign was detected between %s."), hm_found_desc.get_pos_str(key_str))
        return [message, null]
      }
      return [null, d[0]]
    } else if(desc_name.slice(0,2)=="?s") {
      local idx = desc_name.slice(2).tointeger()
      local d = hm_sign_selector().get_desc(idx)
      if(d==null) {
        local message = format(translate("Selected sign %s is not available."), desc_name.slice(2))
        return [message, null]
      }
      return [null, d]
    } else {
      local d = hm_get_sign_desc(desc_name, wtype)
      if(d==null) {
        local message = format(translate("Sign %s (%s) is not found!"), translate(desc_name), desc_name)
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
    // check if the way exists
    local s_pos = origin + pos
    for(local i=0; i<num_exec; i++) {
      try {
        local err = command_x.build_sign_at(player, s_pos, desc)
      } catch(e) {
        local message = format(translate("Error constructing signal on tile %s."), coord3d_to_string(s_pos))
        return message
      }
    }
    return null
  }
}
