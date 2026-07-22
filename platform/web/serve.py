#!/usr/bin/env python3
import argparse
import http.server
import pathlib


class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Resource-Policy", "same-origin")
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


parser = argparse.ArgumentParser()
parser.add_argument("directory", type=pathlib.Path)
parser.add_argument("--port", type=int, default=8000)
arguments = parser.parse_args()
handler = lambda *values, **options: Handler(
    *values, directory=str(arguments.directory), **options)
http.server.ThreadingHTTPServer(("127.0.0.1", arguments.port), handler).serve_forever()
