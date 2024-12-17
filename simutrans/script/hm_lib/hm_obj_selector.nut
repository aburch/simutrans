
class hm_selector_base {
  messages = [] // text to show when users select the way
  descs = []
  
  function get_desc(idx) {
    // given idx can be out of range.
    return idx <= descs.len() ? descs[idx-1] : null
  }
  
  function need_selection() {
    return descs.len()<messages.len()
  }
  
  function _select_intern(player, pos, typ, str) {
    local mo = tile_x(pos.x, pos.y, pos.z).find_object(typ)
    local hasOwned = mo && mo.get_owner().get_name() == player.get_name()
    if (hasOwned) {
      descs.append(mo.get_desc())
      gui.add_message_at(player, hm_obj_selector().get_selection_message(), coord3d(0,0,0))
      return null
    } else {
      return "No valid " +  str + " on " + pos.tostring()
    }
  }
}

class hm_way_selector extends hm_selector_base {
  messages = []
  descs = []
  function select(player, pos) {
    return _select_intern(player, pos, mo_way, "way")
  }
}

class hm_wayobj_selector extends hm_selector_base {
  messages = []
  descs = []
  function select(player, pos) {
    return _select_intern(player, pos, mo_wayobj, "wayobj")
  }
}

class hm_sign_selector extends hm_selector_base {
  messages = []
  descs = []
  function select(player, pos) {
    local tile = tile_x(pos.x, pos.y, pos.z);
    local moSign = tile.find_object(mo_signal);
    if (moSign) {
      descs.append(moSign.get_desc())
    }
    local moRSign = tile.find_object(mo_roadsign);
    if (moRSign) {
      descs.append(moRSign.get_desc())
    }
    if(moSign || moRSign) {
      gui.add_message_at(player, hm_obj_selector().get_selection_message(), coord3d(0,0,0))
      return null
    }
    return "No valid sign on " + pos.tostring()
  }
}

class hm_station_selector extends hm_selector_base {
  messages = []
  descs = []
  function select(player, pos) {
    return _select_intern(player, pos, mo_building, "station")
  }
}

class hm_obj_selector {
  selectors = [hm_way_selector(), hm_wayobj_selector(), hm_sign_selector(), hm_station_selector()]
  
  function need_selection() {
    foreach (s in selectors) {
      if(s.need_selection()) {
        return true
      }
    }
    return false
  }
  
  // generate the message for the next click operation
  function get_selection_message() {
    foreach (s in selectors) {
      if(s.need_selection()) {
        return s.messages[s.descs.len()]
      }
    }
    return "Select the pos to construct."
  }
  
  function select_obj(player, pos) {
    foreach (s in selectors) {
      if(s.need_selection()) {
        return s.select(player, pos)
      }
    }
    return get_selection_message()
  }
}

function hm_select_way(text) {
  hm_way_selector.messages.append(text)
}

function hm_select_wayobj(text) {
  hm_wayobj_selector.messages.append(text)
}

function hm_select_sign(text) {
  hm_sign_selector.messages.append(text)
}

function hm_select_station(text) {
  hm_station_selector.messages.append(text)
}
