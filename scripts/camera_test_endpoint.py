#!/usr/bin/env python3
"""Controllable camera endpoint for repeatable panel performance testing.

Query parameters:
  header_delay_ms=250   wait before sending response headers
  chunk_delay_ms=50     wait between response chunks
  chunks=4              split the payload into this many chunks
  status=503            return a chosen HTTP status
  malformed=1           return bytes that are not a JPEG
"""

from __future__ import annotations

import argparse
import base64
import contextlib
import http.server
import threading
import time
import urllib.error
import urllib.parse
import urllib.request


# A tiny valid JPEG is enough for network timing and failure-path tests. Real
# camera samples can be supplied with --image when decode/scale timing matters.
DEFAULT_JPEG = base64.b64decode(
    "/9j/4AAQSkZJRgABAQEASABIAAD/2wBDAP//////////////////////////////"
    "////////////////////////////////////////////2wBDAf//////////////"
    "////////////////////////////////////////////wAARCAABAAEDASIAAhEB"
    "AxEB/8QAFQABAQAAAAAAAAAAAAAAAAAAAAf/xAAUEAEAAAAAAAAAAAAAAAAAAAAA"
    "/9oADAMBAAIQAxAAAAF//8QAFBABAAAAAAAAAAAAAAAAAAAAAP/aAAgBAQABBQJ//"
    "8QAFBEBAAAAAAAAAAAAAAAAAAAAAP/aAAgBAwEBPwF//8QAFBEBAAAAAAAAAAAA"
    "AAAAAAAAAP/aAAgBAgEBPwF//8QAFBABAAAAAAAAAAAAAAAAAAAAAP/aAAgBAQAG"
    "PwJ//8QAFBABAAAAAAAAAAAAAAAAAAAAAP/aAAgBAQABPyF//9oADAMBAAIAAwAA"
    "ABCf/8QAFBEBAAAAAAAAAAAAAAAAAAAAAP/aAAgBAwEBPxB//8QAFBEBAAAAAAAA"
    "AAAAAAAAAAAAAP/aAAgBAgEBPxB//8QAFBABAAAAAAAAAAAAAAAAAAAAAP/aAAgB"
    "AQE/EEf/2Q=="
)


class CameraHandler(http.server.BaseHTTPRequestHandler):
    payload = DEFAULT_JPEG

    def do_GET(self) -> None:  # noqa: N802 - required by BaseHTTPRequestHandler
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path not in ("/", "/camera.jpg"):
            self.send_error(404)
            return

        query = urllib.parse.parse_qs(parsed.query)
        status = self._number(query, "status", 200, 100, 599)
        header_delay = self._number(query, "header_delay_ms", 0, 0, 60_000) / 1000
        chunk_delay = self._number(query, "chunk_delay_ms", 0, 0, 60_000) / 1000
        chunks = self._number(query, "chunks", 1, 1, 128)
        payload = b"not-a-jpeg" if query.get("malformed", ["0"])[0] == "1" else self.payload

        time.sleep(header_delay)
        self.send_response(status)
        self.send_header("Content-Type", "image/jpeg")
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        if status < 200 or status >= 300:
            return

        chunk_size = max(1, (len(payload) + chunks - 1) // chunks)
        for offset in range(0, len(payload), chunk_size):
            try:
                self.wfile.write(payload[offset : offset + chunk_size])
                self.wfile.flush()
            except (BrokenPipeError, ConnectionResetError):
                break
            if offset + chunk_size < len(payload):
                time.sleep(chunk_delay)

    @staticmethod
    def _number(query: dict[str, list[str]], key: str, default: int,
                minimum: int, maximum: int) -> int:
        try:
            value = int(query.get(key, [str(default)])[0])
        except ValueError:
            value = default
        return max(minimum, min(value, maximum))

    def log_message(self, format: str, *args: object) -> None:
        return


def run_self_test() -> None:
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), CameraHandler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    base = f"http://127.0.0.1:{server.server_port}/camera.jpg"
    try:
        with urllib.request.urlopen(base, timeout=2) as response:
            assert response.status == 200
            assert response.read().startswith(b"\xff\xd8")

        started = time.monotonic()
        with urllib.request.urlopen(
            f"{base}?header_delay_ms=30&chunk_delay_ms=15&chunks=3", timeout=2
        ) as response:
            response.read()
        assert time.monotonic() - started >= 0.05

        with contextlib.suppress(urllib.error.HTTPError):
            urllib.request.urlopen(f"{base}?status=503", timeout=2)
        try:
            urllib.request.urlopen(f"{base}?status=503", timeout=2)
        except urllib.error.HTTPError as error:
            assert error.code == 503
        else:
            raise AssertionError("expected a 503 response")

        with urllib.request.urlopen(f"{base}?malformed=1", timeout=2) as response:
            assert response.read() == b"not-a-jpeg"
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=2)
    print("Camera test endpoint self-test passed")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--image", help="serve this JPEG instead of the built-in sample")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        run_self_test()
        return
    if args.image:
        with open(args.image, "rb") as image_file:
            CameraHandler.payload = image_file.read()
    server = http.server.ThreadingHTTPServer((args.host, args.port), CameraHandler)
    print(f"Camera test endpoint: http://{args.host}:{args.port}/camera.jpg")
    server.serve_forever()


if __name__ == "__main__":
    main()
