#!/usr/bin/env python3
"""Exercise the production RGB buffer bounds and dimension validation."""
import argparse
import ast
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--display-module", type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
tree = ast.parse((args.display_module or root / "components/mipi_rgb/display.py").read_text())
model = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == "model_schema")
schema = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == "_config_schema")
bounds = None
for node in ast.walk(model):
    if not isinstance(node, ast.Dict):
        continue
    for key, value in zip(node.keys, node.values):
        if (isinstance(key, ast.Call) and key.args and
                isinstance(key.args[0], ast.Name) and key.args[0].id == "CONF_BOUNCE_BUFFER_ROWS"):
            bounds = value
assert bounds is not None
dimension_check = next(n for n in schema.body if isinstance(n, ast.If) and
                       "CONF_BOUNCE_BUFFER_ROWS" in ast.unparse(n.test))


class Invalid(ValueError):
    pass


class Validators:
    Invalid = Invalid

    @staticmethod
    def int_range(min, max):
        def validate(value):
            if not min <= value <= max:
                raise Invalid("Out of range")
            return value
        return validate


environment = {"cv": Validators, "CONF_BOUNCE_BUFFER_ROWS": "bounce_buffer_rows"}
validate = eval(compile(ast.Expression(bounds), "production-buffer-bounds", "eval"), environment)
check = compile(ast.Module(body=[dimension_check], type_ignores=[]), "production-buffer-dimensions", "exec")
for height, accepted, rejected in (
    (480, (0, 1, 10, 15, 20, 30, 40, 48, 60, 80, 120, 240), (-1, 7, 11, 127, 255, 256)),
    (272, (0, 1, 2, 4, 8, 16, 17, 34, 68, 136), (-1, 20, 30, 255, 256)),
):
    for rows in (*accepted, *rejected):
        try:
            value = validate(rows)
            exec(check, dict(environment, height=height, config={"bounce_buffer_rows": value}))
        except Invalid:
            assert rows in rejected, (height, rows)
        else:
            assert rows in accepted, (height, rows)
print("RGB buffers: direct DMA zero, divisible positive rows and invalid row counts passed")
