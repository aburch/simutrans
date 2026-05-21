#!/usr/bin/env python3
# NWC_TOOL wire pin + tripwire for the forged-our_client_id crash in
# nwc_tool_t::clone (src/simutrans/network/network_cmd_ingame.cc).
# Pre-fix the wire-supplied our_client_id was fed straight into
# socket_list_t::get_client; 0xFFFFFFFF tripped vector_tpl bounds,
# any valid-other-slot id let the sender impersonate that client.
# The fix in network_command_t::read_from_packet now overrides
# our_client_id with the socket-derived id on the server side, so
# the wire value is informational only.

import struct
import unittest

from . import wire


# nwc_tool_t wire layout (NETWORK_VERSION=1), built per
# network_cmd_ingame.cc:nwc_tool_t::rdwr.
def build(*,
          our_client_id: int,
          tool_id: int = 0x1000,
          player_nr: int = 1,
          pos_x: int = 0,
          pos_y: int = 0,
          pos_z: int = 0,
          wt: int = -1,
          default_param: bytes = b"",
          init: bool = True,
          tool_client_id: int | None = None,
          flags: int = 0,
          callback_id: int = 0,
          sync_step: int = 0,
          map_counter: int = 0,
          exec_flag: bool = True,
          last_sync_step: int = 0,
          custom_data: bytes = b"") -> bytes:
    if tool_client_id is None:
        tool_client_id = our_client_id
    body = struct.pack("<I", our_client_id)                  # network_command_t
    body += struct.pack("<II", sync_step, map_counter)       # network_world_command_t
    body += struct.pack("<B", 1 if exec_flag else 0)         # network_broadcast_world_command_t
    body += struct.pack("<I", last_sync_step)                # nwc_tool_t
    body += struct.pack("<IIHHH", 0, 0, 0, 0, 0)             # checklist_t
    body += struct.pack("<B", player_nr & 0xFF)
    body += struct.pack("<hhb", pos_x, pos_y, pos_z)
    body += struct.pack("<Hh", tool_id, wt)
    body += struct.pack("<H", len(default_param)) + default_param
    body += struct.pack("<B", 1 if init else 0)
    body += struct.pack("<I", tool_client_id)
    body += struct.pack("<B", flags & 0xFF)
    body += struct.pack("<I", callback_id)
    body += custom_data
    return wire.pack_header(wire.NWC_TOOL, body)


class ToolTest(wire.ServerTestCase, unittest.TestCase):

    def test_forged_client_id_does_not_crash(self):
        """Pre-auth NWC_TOOL with our_client_id=0xFFFFFFFF: server
        must absorb the packet and keep ticking, not trip vector_tpl
        bounds in socket_list_t::get_client."""
        wire.assert_heartbeat(self.srv, build(our_client_id=0xFFFFFFFF))

    def test_zero_client_id_does_not_crash(self):
        """Negative control: our_client_id=0 is the legitimate "no
        client" sentinel many clients send before NWC_READY."""
        wire.assert_heartbeat(self.srv, build(our_client_id=0))
