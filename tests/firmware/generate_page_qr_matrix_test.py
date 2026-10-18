#!/usr/bin/env python3
"""Use the production compact encoder as the native QR pixel reference."""
import argparse
from pathlib import Path
from generate_climate_update_test import definition
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',required=True,type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
source=(root/'components/espcontrol/button_grid_wifi_qr_driver.h').read_text()
legacy=definition(source,'wifi_qr_driver_update_compact').replace('wifi_qr_driver_update_compact','legacy_compact')
template=Path(__file__).with_name('page_qr_matrix_native_test.cpp.in').read_text()
args.output.write_text('// Generated from the production compact QR encoder.\n'+template.replace('// @legacy-compact',legacy))
