#!/usr/bin/env python3
"""Behavioral tests for release provenance, compatibility, and artifact tampering."""
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from companion_release import ROOT, compatibility, manifest, verify


class CompanionReleaseTests(unittest.TestCase):
    def test_published_combination_and_tampering(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            firmware = root / 'firmware'; firmware.mkdir()
            companion = root / 'companion'; companion.mkdir()
            revision = 'a' * 40
            version = 'v9.8.7'
            (firmware / 'release-manifest.json').write_text(json.dumps({'sourceRevision': revision, 'releaseVersion': version}))
            for slug in compatibility()['supportedDevices']:
                for suffix in ('.ota.bin', '.factory.bin'):
                    (firmware / (slug + suffix)).write_bytes(b'firmware fixture')
            for suffix in ('.zip', '.dmg'):
                (companion / ('Companion' + suffix)).write_bytes(b'mac fixture')
            path = companion / 'companion-compatibility.json'
            record = manifest(firmware, companion, revision, version)
            self.assertEqual(record['supportedDevices'], ['guition-esp32-s3-4848s040'])
            self.assertEqual(len(record['artifacts']), 4)
            self.assertEqual(record['supportedCombinations'], [{'firmwareProtocol': 3, 'companionProtocol': 3}])
            path.write_text(json.dumps(record))
            verify(path, firmware, companion, revision, version)
            with self.assertRaises(ValueError): verify(path, firmware, companion, 'b' * 40, version)
            with self.assertRaises(ValueError): verify(path, firmware, companion, revision, 'v9.8.8')
            (companion / 'Companion.zip').write_bytes(b'tampered bytes')
            with self.assertRaises(ValueError): verify(path, firmware, companion, revision, version)
            (companion / 'Companion.zip').write_bytes(b'mac fixture')
            record['protocolVersion'] = 2
            path.write_text(json.dumps(record))
            with self.assertRaises(ValueError): verify(path, firmware, companion, revision, version)
            (companion / 'Extra.zip').write_bytes(b'extra build')
            with self.assertRaises(ValueError): manifest(firmware, companion, revision, version)

    def test_incomplete_and_unpinned_release(self):
        with TemporaryDirectory() as temporary:
            directory = Path(temporary)
            with self.assertRaises(ValueError): manifest(directory, directory, 'main', 'v9.8.7')
            with self.assertRaises(ValueError): manifest(directory, directory, 'a' * 40, 'dev')
            with self.assertRaises(FileNotFoundError): manifest(directory, directory, 'a' * 40, 'v9.8.7')


if __name__ == '__main__':
    unittest.main()
