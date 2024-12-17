// originally obtained from https://github.com/128na/sugoi-simutrans-squirrel-scripts/blob/master/lib_obj_finder_v2.nut at 2021/01/20
// some functions were modified by himeshi

class ObjFinder {
    player = null;
    area = null;

    constructor(pl, a = [
        [0, 0, -2],
        [10, 10, 2]
    ]) {
        player = pl;
        area = a;
    }

    // 指定値間の数値をイテレートする
    function _stepGenerator(from, to) {
        if (from < to) {
            for (local i = from; i <= to; i++) {
                yield i;
            }

        } else {
            for (local i = from; i >= to; i--) {
                yield i;
            }
        }
    }

    // 指定座標空間をイテレートする
    function _coord3dGenerator() {
        foreach(x in _stepGenerator(area[0][0], area[1][0])) {
            foreach(y in _stepGenerator(area[0][1], area[1][1])) {
                foreach(z in _stepGenerator(area[0][2], area[1][2])) {
                    yield coord3d(x, y, z);
                }
            }
        }
    }

    // 指定名のオブジェクトが指定座標空間にないときのメッセージを返す
    function getMissingObjText(type) {
        return "範囲" + "[" + coord3d(area[0][0], area[0][1], area[0][2]) +
            "] - [" + coord3d(area[1][0], area[1][1], area[1][2]) + "]" + "に" + type + "がありません。";
    }

    // 軌道を指定座標空間から探す
    function findWay() {
        foreach(pos in _coord3dGenerator()) {
            local tile = tile_x(pos.x, pos.y, pos.z);
            gui.add_message_at(player, "findWay() coord :" + coord3d_to_string(tile), world.get_time())
            local moWay = tile.find_object(mo_way);

            local hasOwnedWay = moWay && moWay.get_owner().get_name() == player.get_name();
            if (hasOwnedWay) {
                return moWay.get_desc();
            }
        }
    }


    // 架線を指定座標空間から探す
    function findWayObj() {
        foreach(pos in _coord3dGenerator()) {
            local tile = tile_x(pos.x, pos.y, pos.z);
            local moWobj = tile.find_object(mo_wayobj);
            local hasOwnedWay = moWobj && moWobj.get_owner().get_name() == player.get_name();
            if (hasOwnedWay) {
                return moWobj.get_desc();
            }
        }
    }

    // 標識を指定座標空間から探す
    function findSign() {
        foreach(pos in _coord3dGenerator()) {
            local tile = tile_x(pos.x, pos.y, pos.z);
            local moSign = tile.find_object(mo_signal);
            if (moSign) {
                return moSign.get_desc();
            }
            local moRSign = tile.find_object(mo_roadsign);
            if (moRSign) {
                return moRSign.get_desc();
            }
        }
    }

    // プラットフォームを指定座標空間から探す
    function findPlatform() {
        foreach(pos in _coord3dGenerator()) {
            local tile = tile_x(pos.x, pos.y, pos.z);
            local moBld = tile.find_object(mo_building);
            local hasOwnedPlatform = moBld &&
                moBld.get_owner().get_name() == player.get_name();
            if (hasOwnedPlatform) {
                return moBld.get_desc();
            }
        }
    }

    // マーカーを指定座標空間から探す
    function findLabel() {
        foreach(pos in _coord3dGenerator()) {
            local tile = tile_x(pos.x, pos.y, pos.z);
            local obj = tile.find_object(mo_label);

            local hasOwnedWay = obj && obj.get_owner().get_name() == player.get_name();
            if (hasOwnedWay) {
                return obj;
            }
        }
    }

    // depot
    function findDepot() {
        foreach(pos in _coord3dGenerator()) {
            local tile = tile_x(pos.x, pos.y, pos.z);
            local moBld = tile.find_object(mo_depot);
            local hasOwnedDepot = moBld &&
                moBld.get_owner().get_name() == player.get_name();
            if (hasOwnedDepot) {
                return moBld.get_desc();
            }
        }
    }
}
