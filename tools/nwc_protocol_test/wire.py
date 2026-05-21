#!/usr/bin/env python3
# Shared rig for the black-box tests in this package.  Server context
# manager, 6-byte (size, version, id) framing helpers, and the
# assert_heartbeat assertion used by the silent-absorb regression
# tests.  Per-command wire layouts live in test_<command>.py.

import fcntl
import os
import shutil
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PAK_DIR = ROOT / "simutrans" / "pak"
BASE_DIR = ROOT / "simutrans"
EMPTY_SAVE = ROOT / "tests" / "empty-16x16.sve"

NETWORK_VERSION = 1
HEADER_SIZE = 6

# Mirrors network_command_id in src/simutrans/network/network_cmd.h.
NWC_GAMEINFO     = 1
NWC_JOIN         = 4
NWC_SYNC         = 5
NWC_GAME         = 6
NWC_READY        = 7
NWC_TOOL         = 8
NWC_CHECK        = 9
NWC_SERVICE      = 11
NWC_AUTH_PLAYER  = 12
NWC_CHG_PLAYER   = 13
NWC_STEP         = 16

_PACKET_NAMES = {
    NWC_GAMEINFO: "NWC_GAMEINFO",
    NWC_JOIN: "NWC_JOIN",
    NWC_SYNC: "NWC_SYNC",
    NWC_GAME: "NWC_GAME",
    NWC_READY: "NWC_READY",
    NWC_TOOL: "NWC_TOOL",
    NWC_CHECK: "NWC_CHECK",
    NWC_SERVICE: "NWC_SERVICE",
    NWC_AUTH_PLAYER: "NWC_AUTH_PLAYER",
    NWC_CHG_PLAYER: "NWC_CHG_PLAYER",
    NWC_STEP: "NWC_STEP",
}


def pkt_name(pkt_id: int) -> str:
    return f"{_PACKET_NAMES.get(pkt_id, '?')}({pkt_id})"


def find_simutrans() -> Path:
    for candidate in [
        ROOT / "build-headless" / "simutrans" / "simutrans",
        ROOT / "build" / "simutrans" / "simutrans",
        ROOT / "sim",
    ]:
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate
    sys.exit("no simutrans binary found; build with cmake or autoconf first")


def require_pakset():
    if not PAK_DIR.is_dir() or not any(PAK_DIR.iterdir()):
        sys.exit(f"no pakset at {PAK_DIR}; run "
                 "`cd simutrans && ../tools/get_pak.sh pak64` once")
    if not EMPTY_SAVE.is_file():
        sys.exit(f"missing starter savegame {EMPTY_SAVE}")


def _free_port() -> int:
    # Brief TOCTOU between close and simutrans binding; a collision
    # surfaces as the same "did not start listening" error as a real
    # launch failure, which is acceptable.
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


class Server:
    """Context manager spawning a headless server on a free port.
    Each instance gets its own temp userdir so concurrent runs don't
    collide with each other or with a developer's ~/simutrans tree."""

    def __init__(self, port: int | None = None):
        self.port = port if port is not None else _free_port()
        self.proc: subprocess.Popen | None = None
        self.userdir: Path | None = None

    def __enter__(self) -> "Server":
        self.userdir = Path(tempfile.mkdtemp(prefix="nwc-e2e-"))
        save_dir = self.userdir / "save"
        save_dir.mkdir(parents=True)
        shutil.copyfile(EMPTY_SAVE, save_dir / "empty.sve")

        sim = find_simutrans()
        require_pakset()
        env = os.environ.copy()
        env.setdefault("SDL_VIDEODRIVER", "dummy")
        args = [
            str(sim),
            "-set_basedir", str(BASE_DIR),
            "-set_userdir", str(self.userdir),
            "-objects", "pak",
            "-nomidi", "-nosound", "-mute",
            "-server", str(self.port),
            "-load", "empty",
            "-debug", "2",
        ]
        self.proc = subprocess.Popen(
            args, env=env,
            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
        )
        # Non-blocking so read_stderr_tail won't wait for a full read
        # or process exit when called from an assertion message.
        fd = self.proc.stderr.fileno()
        fcntl.fcntl(fd, fcntl.F_SETFL,
                    fcntl.fcntl(fd, fcntl.F_GETFL) | os.O_NONBLOCK)
        if not self._wait_listening(timeout=20.0):
            stderr = self.read_stderr_tail(limit=8192)
            self.__exit__(None, None, None)
            raise RuntimeError(
                f"server did not start listening on {self.port} within "
                f"20s; stderr tail:\n{stderr}"
            )
        return self

    def __exit__(self, *exc):
        if self.proc is not None:
            if self.proc.poll() is None:
                self.proc.send_signal(signal.SIGTERM)
                try:
                    self.proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    self.proc.kill()
                    self.proc.wait()
            if self.proc.stderr is not None:
                self.proc.stderr.close()
        if self.userdir and self.userdir.exists():
            shutil.rmtree(self.userdir, ignore_errors=True)

    def _wait_listening(self, timeout: float) -> bool:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.proc and self.proc.poll() is not None:
                return False
            try:
                with socket.create_connection(("127.0.0.1", self.port),
                                              timeout=0.5):
                    return True
            except OSError:
                time.sleep(0.2)
        return False

    def alive(self) -> bool:
        """Process running AND port accepting new connections."""
        if self.proc is None or self.proc.poll() is not None:
            return False
        try:
            with socket.create_connection(("127.0.0.1", self.port),
                                          timeout=1.0):
                return True
        except OSError:
            return False

    def read_stderr_tail(self, limit: int = 4096) -> str:
        if not self.proc or not self.proc.stderr:
            return ""
        try:
            chunk = os.read(self.proc.stderr.fileno(), limit)
        except BlockingIOError:
            return ""
        return chunk.decode(errors="replace")[-2000:]


def pack_header(packet_id: int, body: bytes) -> bytes:
    # `size` is the total packet length including header — matches
    # network_command_t::rdwr on the wire.
    size = HEADER_SIZE + len(body)
    return struct.pack("<HHH", size, NETWORK_VERSION, packet_id) + body


def unpack_header(data: bytes) -> tuple[int, int, int]:
    if len(data) < HEADER_SIZE:
        raise AssertionError(
            f"need {HEADER_SIZE}B for header, got {len(data)}: {data.hex()}"
        )
    return struct.unpack("<HHH", data[:HEADER_SIZE])


def recv_exact(sock: socket.socket, n: int, timeout: float) -> bytes:
    sock.settimeout(timeout)
    buf = b""
    while len(buf) < n:
        try:
            chunk = sock.recv(n - len(buf))
        except (socket.timeout, OSError):
            break
        if not chunk:
            break
        buf += chunk
    return buf


def read_packets(sock: socket.socket, timeout: float,
                 max_packets: int = 8) -> list[tuple[int, bytes]]:
    """Drain whole packets from `sock` as `(packet_id, body)` tuples.
    Stops on close, timeout, or after `max_packets`."""
    sock.settimeout(timeout)
    out: list[tuple[int, bytes]] = []
    buf = b""
    deadline = time.monotonic() + timeout
    while len(out) < max_packets and time.monotonic() < deadline:
        try:
            chunk = sock.recv(4096)
        except (socket.timeout, OSError):
            break
        if not chunk:
            break
        buf += chunk
        while True:
            if len(buf) < HEADER_SIZE:
                break
            size, version, pkt_id = struct.unpack("<HHH", buf[:HEADER_SIZE])
            if size < HEADER_SIZE:
                raise AssertionError(
                    f"impossible packet size={size}: {buf[:HEADER_SIZE].hex()}"
                )
            if len(buf) < size:
                break
            if version != NETWORK_VERSION:
                raise AssertionError(
                    f"version={version} (expected {NETWORK_VERSION}); "
                    f"raw: {buf[:size].hex()}"
                )
            out.append((pkt_id, buf[HEADER_SIZE:size]))
            buf = buf[size:]
            if len(out) >= max_packets:
                break
    return out


def send_and_recv_exact(srv: "Server", pkt: bytes, n: int,
                        recv_timeout: float = 3.0,
                        connect_timeout: float = 2.0) -> bytes:
    with socket.create_connection(("127.0.0.1", srv.port),
                                  timeout=connect_timeout) as s:
        s.sendall(pkt)
        return recv_exact(s, n, timeout=recv_timeout)


def assert_heartbeat(srv: "Server", pkt: bytes,
                     drain_seconds: float = 3.0,
                     min_steps: int = 1) -> None:
    """Send `pkt`, drain the reply, require ≥`min_steps` NWC_STEP
    heartbeats and a still-alive server.  The "silently absorbs a
    malicious packet and keeps ticking" assertion."""
    try:
        with socket.create_connection(("127.0.0.1", srv.port),
                                      timeout=2.0) as s:
            s.sendall(pkt)
            packets = read_packets(s, timeout=drain_seconds, max_packets=16)
    except OSError as e:
        raise AssertionError(
            f"send failed: {e}; stderr tail:\n{srv.read_stderr_tail()}"
        )

    steps = [p for p in packets if p[0] == NWC_STEP]
    if len(steps) < min_steps:
        names = [pkt_name(pid) for pid, _ in packets]
        raise AssertionError(
            f"saw {len(steps)} NWC_STEP heartbeat(s), wanted "
            f">= {min_steps}; received = {names}; "
            f"stderr tail:\n{srv.read_stderr_tail()}"
        )
    if not srv.alive():
        raise AssertionError(
            "heartbeat seen but server failed to accept a new "
            "connection after; stderr tail:\n" + srv.read_stderr_tail()
        )


class ServerTestCase:
    """Mixin: fresh `Server` per test method on `self.srv`.  Per-test
    isolation is deliberate — a packet that crashes the server must
    not poison the next test."""

    def setUp(self):
        super().setUp()
        self.srv = self.enterContext(Server())
