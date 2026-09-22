"""Vendor a pinned integration catalog contract, or verify the checked-in copy."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
LOCK = ROOT / "product/ha_catalog/contract-lock.json"
FILES = {
    "protocol/catalog-v1.json": "product/ha_catalog/catalog-v1.json",
    "protocol/fixtures/catalog-v1.json": "product/ha_catalog/fixtures/catalog-v1.json",
    "protocol/generated/catalog.ts": "src/webserver/generated/ha_catalog_contract.ts",
    "protocol/generated/catalog.h": "components/espcontrol/ha_catalog_contract.h",
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-checkout", type=Path)
    args = parser.parse_args()
    if args.source_checkout:
        source = args.source_checkout.resolve()
        if subprocess.check_output(["git", "status", "--porcelain", "--", *FILES], cwd=source).strip():
            raise SystemExit("Commit upstream contract files before pinning them")
        revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=source, text=True).strip()
        lock = {"repository": "https://github.com/jtenniswood/espcontrol-integration", "revision": revision, "files": {}}
        for upstream, local in FILES.items():
            data = (source / upstream).read_bytes()
            destination = ROOT / local
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
            lock["files"][local] = {"upstream": upstream, "sha256": digest(data)}
        LOCK.write_text(json.dumps(lock, indent=2) + "\n")
    lock = json.loads(LOCK.read_text())
    if len(lock["revision"]) != 40 or set(lock["files"]) != set(FILES.values()):
        raise SystemExit("Invalid upstream contract pin")
    for local, item in lock["files"].items():
        if digest((ROOT / local).read_bytes()) != item["sha256"]:
            raise SystemExit(f"Vendored contract drift: {local}; synchronize from the pinned integration revision")
    print(f"Catalog contract pinned to {lock['revision']}")


if __name__ == "__main__":
    main()
