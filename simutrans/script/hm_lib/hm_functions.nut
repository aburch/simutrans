
this.hm_all_waytypes <- [wt_road,wt_rail,wt_water,wt_monorail,wt_maglev,wt_tram,wt_narrowgauge,wt_air,wt_power]

function hm_get_way_desc(name) {
  foreach(wt in hm_all_waytypes) {
    foreach (st in [st_flat, st_elevated, st_tram]) {
      foreach (w in way_desc_x.get_available_ways(wt, st)) {
        if(w.get_name()==desc_name) {
          return w
        }
      }
    }
  }
  return null
}

function hm_get_bridge_desc(name) {
  foreach(wt in hm_all_waytypes) {
    foreach (b in bridge_desc_x.get_available_bridges(wt)) {
      if(b.get_name()==desc_name) {
        return b
      }
    }
  }
  return null
}

function hm_get_sign_desc(name) {
  foreach(wt in hm_all_waytypes) {
    foreach (s in sign_desc_x.get_available_signs(wt)) {
      if(s.get_name()==desc_name) {
        return s
      }
    }
  }
  return null
}
