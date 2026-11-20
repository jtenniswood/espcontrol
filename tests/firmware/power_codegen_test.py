"""Exercise the native Power Flow widget dependency against pinned ESPHome."""
import ast
import importlib.util
from pathlib import Path

from esphome.core import CORE
from esphome.components.lvgl.defines import get_lv_uses

path = Path(__file__).resolve().parents[2] / 'components/espcontrol/__init__.py'
spec = importlib.util.spec_from_file_location('power_espcontrol_codegen', path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
CORE.reset()
module.register_power_flow_widgets()
assert 'line' in get_lv_uses()
module.register_power_flow_widgets()
assert len([use for use in get_lv_uses() if use == 'line']) == 1
# Ensure the tested hook is actually called by component code generation.
tree = ast.parse(path.read_text(encoding='utf-8'))
generate = next(n for n in tree.body if isinstance(n, ast.AsyncFunctionDef) and n.name == 'to_code')
assert isinstance(generate.body[0], ast.Expr)
assert isinstance(generate.body[0].value, ast.Call)
assert generate.body[0].value.func.id == 'register_power_flow_widgets'
print('Power Flow native LVGL line dependency passed.')
