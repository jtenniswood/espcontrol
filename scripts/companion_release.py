#!/usr/bin/env python3
"""Record and verify a source-pinned firmware/Companion release combination."""
from __future__ import annotations
import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def compatibility(root=ROOT):
    contract = json.loads((root / 'product/v2/companion_capabilities.json').read_text())
    devices = {path.stem: json.loads(path.read_text()) for path in (root / 'product/v2/devices').glob('*.json')}
    return {
        'schemaVersion': 1,
        'protocolVersion': contract['protocol']['version'],
        'capabilityVersion': contract['version'],
        'contractSha256': digest(root / 'product/v2/companion_capabilities.json'),
        'supportedDevices': sorted(slug for slug, device in devices.items() if device.get('config', {}).get('capabilities', {}).get('companion')),
        'supportedCombinations': [{'firmwareProtocol': contract['protocol']['version'],
                                   'companionProtocol': contract['protocol']['version']}],
        'releasePolicy': 'coordinated',
        'savedPanelConfigVersion': json.loads((root / 'product/release_contract.json').read_text())['panelConfigDocumentVersion'],
        'wireFixturesSha256': digest(root / 'compatibility/fixtures/companion_protocol_v3.json'),
    }


def manifest(firmware_dir, companion_dir, source_revision, version, root=ROOT):
    if not re.fullmatch(r'[0-9a-f]{40}', source_revision):
        raise ValueError('A full source revision is required')
    if not re.fullmatch(r'v?\d+\.\d+\.\d+', version):
        raise ValueError('A stable release version is required')
    contract = compatibility(root)
    provenance = json.loads((firmware_dir / 'release-manifest.json').read_text())
    if provenance.get('sourceRevision') != source_revision or provenance.get('releaseVersion') != version:
        raise ValueError('Firmware release provenance does not match the Companion release')
    artifacts = {}
    for slug in contract['supportedDevices']:
        for suffix in ('.ota.bin', '.factory.bin'):
            path = firmware_dir / (slug + suffix)
            artifacts[path.name] = {'sha256': digest(path), 'bytes': path.stat().st_size}
    for suffix in ('.zip', '.dmg'):
        matches = list(companion_dir.glob('*' + suffix))
        if len(matches) != 1:
            raise ValueError(f'Expected exactly one Companion {suffix} artifact')
        path = matches[0]
        artifacts[path.name] = {'sha256': digest(path), 'bytes': path.stat().st_size}
    return {**contract, 'sourceRevision': source_revision, 'releaseVersion': version,
            'artifacts': artifacts,
            'validation': {'automated': 'release workflow checks', 'physicalDeviceTesting': 'not asserted'}}


def verify(path, firmware_dir, companion_dir, source_revision, version, root=ROOT):
    expected = manifest(firmware_dir, companion_dir, source_revision, version, root)
    if json.loads(path.read_text()) != expected:
        raise ValueError('Companion compatibility manifest or release artifacts do not match')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['create', 'verify'])
    parser.add_argument('--firmware-dir', type=Path, required=True)
    parser.add_argument('--companion-dir', type=Path, required=True)
    parser.add_argument('--source-revision', required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    if args.command == 'create':
        args.out.write_text(json.dumps(manifest(args.firmware_dir, args.companion_dir, args.source_revision, args.version), indent=2) + '\n')
    else:
        verify(args.out, args.firmware_dir, args.companion_dir, args.source_revision, args.version)


if __name__ == '__main__':
    main()
