#!/usr/bin/env python3
"""Small JSON-RPC client for Simutrans' built-in MCP server."""

from __future__ import annotations

import argparse
import base64
import json
import socket
import sys
import time
from pathlib import Path


def connect(hosts: list[str], port: int, timeout: float) -> socket.socket:
    last_error: OSError | None = None
    deadline = time.time() + timeout
    while time.time() < deadline:
        for host in hosts:
            try:
                return socket.create_connection((host, port), timeout=1.0)
            except OSError as exc:
                last_error = exc
        time.sleep(0.25)
    if last_error is not None:
        raise last_error
    raise TimeoutError("could not connect")


def send(sock: socket.socket, message: dict) -> None:
    sock.sendall((json.dumps(message) + "\n").encode("utf-8"))


def read_responses(sock: socket.socket, wanted_ids: set[int], timeout: float) -> dict[int, dict]:
    deadline = time.time() + timeout
    buffer = b""
    responses: dict[int, dict] = {}
    sock.settimeout(0.2)
    while time.time() < deadline and not wanted_ids.issubset(responses):
        try:
            chunk = sock.recv(262144)
            if not chunk:
                break
            buffer += chunk
        except socket.timeout:
            continue

        while b"\n" in buffer:
            line, buffer = buffer.split(b"\n", 1)
            if not line.strip():
                continue
            response = json.loads(line)
            response_id = response.get("id")
            if isinstance(response_id, int):
                responses[response_id] = response
    return responses


def content_text(response: dict) -> str:
    content = response.get("result", {}).get("content", [])
    if not content:
        return json.dumps(response, ensure_ascii=False)
    first = content[0]
    if first.get("type") == "text":
        return first.get("text", "")
    return json.dumps(response, ensure_ascii=False)


def save_capture(response: dict, output: Path) -> None:
    content = response.get("result", {}).get("content", [])
    if not content or content[0].get("type") != "image":
        raise ValueError("capture_screen did not return image content")
    output.write_bytes(base64.b64decode(content[0]["data"]))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--host", action="append", help="host to try; defaults to ::1 and 127.0.0.1")
    parser.add_argument("--player-nr", type=int, default=0)
    parser.add_argument("--run-squirrel", help="Squirrel code to execute through MCP run_squirrel")
    parser.add_argument("--capture", type=Path, help="write MCP capture_screen PNG to this path")
    parser.add_argument("--list-tools", action="store_true")
    parser.add_argument("--timeout", type=float, default=8.0)
    args = parser.parse_args()

    hosts = args.host or ["::1", "127.0.0.1"]
    sock = connect(hosts, args.port, args.timeout)
    next_id = 1

    send(sock, {
        "jsonrpc": "2.0",
        "id": next_id,
        "method": "initialize",
        "params": {
            "protocolVersion": "2024-11-05",
            "capabilities": {},
            "clientInfo": {"name": "simutrans-mcp-gui-debug", "version": "1"},
        },
    })
    wanted = {next_id}
    next_id += 1

    list_id = None
    if args.list_tools:
        list_id = next_id
        send(sock, {"jsonrpc": "2.0", "id": list_id, "method": "tools/list", "params": {}})
        wanted.add(list_id)
        next_id += 1

    run_id = None
    if args.run_squirrel is not None:
        run_id = next_id
        send(sock, {
            "jsonrpc": "2.0",
            "id": run_id,
            "method": "tools/call",
            "params": {
                "name": "run_squirrel",
                "arguments": {"code": args.run_squirrel, "player_nr": args.player_nr},
            },
        })
        wanted.add(run_id)
        next_id += 1

    printed_run = False
    if run_id is not None and args.capture is not None:
        responses = read_responses(sock, wanted, args.timeout)
        print(content_text(responses[run_id]))
        printed_run = True
        time.sleep(1.0)
        wanted = set()
    else:
        responses = {}

    capture_id = None
    if args.capture is not None:
        capture_id = next_id
        send(sock, {
            "jsonrpc": "2.0",
            "id": capture_id,
            "method": "tools/call",
            "params": {"name": "capture_screen", "arguments": {}},
        })
        wanted.add(capture_id)

    responses.update(read_responses(sock, wanted, args.timeout))
    sock.close()

    if list_id is not None:
        print(json.dumps(responses[list_id], ensure_ascii=False))
    if run_id is not None and run_id in responses and not printed_run:
        print(content_text(responses[run_id]))
    if capture_id is not None:
        save_capture(responses[capture_id], args.capture)
        print(str(args.capture))

    return 0


if __name__ == "__main__":
    sys.exit(main())
