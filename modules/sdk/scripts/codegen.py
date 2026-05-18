"""Shared helpers for PCDOGS SDK code generators."""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path
from types import ModuleType


def load_python_module(path: Path, module_name: str | None = None) -> ModuleType:
    """Import a blueprint as an isolated module so generators can evaluate SDK rows."""

    path = path.resolve()
    spec = importlib.util.spec_from_file_location(module_name or path.stem, path)
    if spec is None or spec.loader is None:
        raise SystemExit(f"unable to import blueprint: {path}")

    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    script_dir = Path(__file__).resolve().parent
    old_sys_path = list(sys.path)
    sys.path[:0] = [str(script_dir), str(path.parent)]
    try:
        spec.loader.exec_module(module)
    finally:
        sys.path[:] = old_sys_path
    return module


def pattern_tokens(pattern: str) -> list[str]:
    """Split a blueprint AOB pattern into the byte and wildcard tokens used by C emitters."""

    return pattern.split()


def c_sig(pattern: str) -> str:
    """Render an AOB pattern as the byte string consumed by the runtime sigscan API."""

    body = "".join(
        "?" if token == "??" else f"\\x{token.upper()}"
        for token in pattern_tokens(pattern)
    )
    return f'"{body}"'


def c_mask(pattern: str) -> str:
    """Render an AOB pattern mask so wildcard bytes stay aligned with the signature string."""

    body = "".join("?" if token == "??" else "x" for token in pattern_tokens(pattern))
    return f'"{body}"'
