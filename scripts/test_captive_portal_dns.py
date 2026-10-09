"""Exercise the production portal DNS implementation using real host UDP."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="espcontrol-captive-dns-") as directory:
    for ipv6 in (0, 1):
        executable = str(Path(directory) / f"dns-{ipv6}")
        subprocess.run(["g++", "-std=c++17", "-DUSE_ESP32", f"-DUSE_NETWORK_IPV6={ipv6}",
                        "-Itests/captive_portal_stubs", "-I.", "tests/captive_portal_dns_tests.cpp",
                        "components/captive_portal/dns_server_esp32_idf.cpp", "-o", executable], cwd=ROOT, check=True)
        subprocess.run([executable], cwd=ROOT, check=True)
