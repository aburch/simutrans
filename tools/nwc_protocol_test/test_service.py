#!/usr/bin/env python3
# NWC_SERVICE wire pin + tripwire for the pre-auth list-shaped OOM
# in nwc_service_t::rdwr (src/simutrans/network/network_cmd.cc).
# Pre-fix the server-side rdwr was wire-symmetric, so a forged
# SRVC_GET_CLIENT_LIST with count=0xFFFFFFFF drove socket_list_t::rdwr
# through ~4 billion allocations until std::bad_alloc.  The
# admin-auth gate in execute() only ran after rdwr returned.  The
# fix makes server-side reads of the list-shaped flags no-ops; only
# the saving (response) leg touches socket_list_t / address_list_t.

import struct
import unittest

from . import wire


SRVC_LOGIN_ADMIN     = 0
SRVC_ANNOUNCE_SERVER = 1
SRVC_GET_CLIENT_LIST = 2
SRVC_GET_BLACK_LIST  = 5


# We always append the wire-controlled count so the packet shape
# exercises the pre-fix vulnerable read path regardless of flag.
def build(*, flag: int, number: int = 0, count: int = 0,
          our_client_id: int = 0) -> bytes:
    body = struct.pack("<I", our_client_id)
    body += struct.pack("<II", flag, number)
    body += struct.pack("<I", count)
    return wire.pack_header(wire.NWC_SERVICE, body)


class ServiceTest(wire.ServerTestCase, unittest.TestCase):

    def test_get_client_list_huge_count_does_not_crash(self):
        """count=0xFFFFFFFF on SRVC_GET_CLIENT_LIST must not trigger
        the 2^32-allocation loop in socket_list_t::rdwr."""
        wire.assert_heartbeat(self.srv, build(flag=SRVC_GET_CLIENT_LIST,
                                              count=0xFFFFFFFF))

    def test_get_black_list_huge_count_does_not_crash(self):
        """Sibling path through address_list_t::rdwr."""
        wire.assert_heartbeat(self.srv, build(flag=SRVC_GET_BLACK_LIST,
                                              count=0xFFFFFFFF))

    def test_unknown_flag_does_not_crash(self):
        """Unknown flag must be absorbed silently — guards against a
        regression that swaps "no-op on loading" for "fatal on
        loading"."""
        wire.assert_heartbeat(self.srv, build(flag=0xDEAD, count=0xFFFFFFFF))
