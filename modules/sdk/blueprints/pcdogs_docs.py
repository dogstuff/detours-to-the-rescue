"""Human-readable documentation helpers for generated PCDOGS blueprint types."""

from __future__ import annotations

import re
from collections.abc import Callable
from typing import Any

_LEVEL_PACKAGE_USE = (
    "level package loading paths that locate material tables, scene graphs, "
    "mesh data, and collision sections"
)

_DOMAIN_USE = {
    "Actor": "spawned/template runtime actor paths for rendering, animation, movement, collision, camera targeting, and per-frame logic",
    "Animation": "animation playback paths for controller chains, keyframes, morph targets, visibility tracks, and mesh pose interpolation",
    "Audio": "Miles/sound playback paths for sample handles, sound descriptors, wave metadata, and runtime sound slots",
    "Camera": "camera update and rendering paths for frustum culling, room visibility, and view-state management",
    "Checkers": "the in-game checkers/minigame board-state logic",
    "Collision": "level collision paths for geometry tests, hit events, responses, and movement blocking",
    "Component": "component attachment paths for spawned objects, collision boxes, mesh components, and trail effects",
    "D3D": "Direct3D device enumeration, capabilities, render-state, and graphics initialization paths",
    "DDraw": "DirectDraw surface, display-mode, pixel-format, and device enumeration paths",
    "DInput": "DirectInput joystick/device enumeration, data-format setup, and raw input polling paths",
    "Entity": "persistent spawn/script/defaults records that own or request active runtime actors",
    "File": "file and package access paths for asset loading, CRT-style file state, and sharing/access modes",
    "Input": "keyboard, joystick, and gamepad processing before movement and menu logic consume per-frame state",
    "Level": _LEVEL_PACKAGE_USE,
    "LevelBlob": _LEVEL_PACKAGE_USE,
    "Material": "material and texture-table paths for package loading, animation frames, and renderer state setup",
    "Menu": "menu and front-end/HUD paths that prepare progress summaries, option rows, prompts, and save/load screens",
    "Math": "geometry and transform paths for collision checks, culling, camera math, and mesh rendering",
    "Mesh": "mesh loading and rendering paths that read vertices, polygons, normals, material refs, and scene-node transforms",
    "Movie": "movie playback paths that open video streams, maintain playback buffers, and handle skip/close state",
    "Nav": "navigation graph and pathfinding paths for actor movement commands and neighbor/path state",
    "Physics": "physics and movement integration paths that carry per-object simulation state",
    "Powerup": "powerup spawn/update paths that walk per-level powerup entries and instantiate template actor records",
    "Pk": "package-resource parsing paths for level, geometry, material, texture, sprite, sound, script, and UI resources",
    "Pkg": "package table-of-contents and resource-record parsing paths used while loading game data archives",
    "Render": "renderer paths for polygon batches, clipping, sprite layers, colors, gradients, and draw work areas",
    "SaveGame": "save/load paths that store slots, current level progress, flags, and persisted game state",
    "Scene": "scene graph loading and traversal paths that link model/object nodes, local transforms, and resource references",
    "Script": "game scripting interpreter paths for opcode dispatch, script contexts, and script-bound entities",
    "Texture": "texture loading/rendering paths that track surface descriptors and package texture metadata",
    "Trail": "trail and bone-effect rendering paths for segmented trails attached to moving objects or bones",
    "UI": "front-end and HUD paths that draw text, lives icons, fireworks, and other interface resources",
}

_SPECIFIC_USE = {
    "D3D_DriverInfo": "DirectDraw/Direct3D driver enumeration record, covering display device selection, hardware acceleration, and display modes.",
    "File_Handle": "CRT-compatible file handle layout, used by package and asset loading streams.",
    "File_OpenMode": "Access/share-mode pair, passed through file-open wrappers.",
    "Input_Event": "Compact input event record; `type` selects the event kind and `value` carries the button/key payload.",
    "Input_State": "Per-frame input snapshot, shared by movement, menus, movie playback, and replay hooks.",
    "Input_JoystickState": "DirectInput-style joystick axis/POV snapshot, later folded into `DTTR_PCDOGS_Input_State`.",
    "DInput_JoystickState": "DirectInput DIJOYSTATE-compatible axis/button snapshot.",
    "DInput_DeviceEnumContext": "DirectInput joystick enumeration context for discovered device GUIDs.",
    "Movie_PlaybackBuffer": "Movie playback buffer state, covering frame reads, input, and close/skip handling.",
    "SaveGame_Data": "Save-file header plus 0x5c-byte save-slot payloads for game state, settings, and player-lives dwords.",
    "SaveGame_Slot": "0x5c-byte per-slot progress payload, used by save/load UI and completion calculations.",
    "Script_OpcodeTable": "Opcode dispatch table, routing bytecode operations to script handlers.",
    "Script_Context": "Script interpreter context for instruction state and game-script execution data.",
}

_KIND_USE = {
    "Header": "Carries offsets, counts, or version fields, and is read before payload parsing.",
    "Entry": "Represents one item, usually inside a subsystem array or table.",
    "Record": "Represents a serialized or runtime record, scoped to the surrounding subsystem.",
    "Descriptor": "Describes resource or runtime options, then feeds object creation.",
    "Definition": "Stores reusable definition data, which runtime objects reference later.",
    "Handle": "Lightweight reference to subsystem-owned runtime state.",
    "State": "Stores mutable runtime state, updated across frames or subsystem calls.",
    "Context": "Groups call-local or interpreter state for subsystem routines.",
    "Slot": "Represents one fixed slot, usually in a subsystem-managed table.",
    "Table": "Groups indexed lookup entries, used by loader, renderer, or scripting code.",
    "Node": "Participates in graph/tree traversal, or in hierarchy updates.",
    "Vertex": "Stores vertex data for mesh, collision, or renderer paths.",
    "Polygon": "Stores polygon/face data for mesh, collision, or renderer paths.",
    "Keyframe": "Stores animation values, sampled by frame interpolation code.",
    "Controller": "Carries animation control state, including blending inputs.",
    "Resource": "Mirrors a package resource payload or metadata block from game data files.",
}

_DOMAIN_PREFIXES = (
    ("Pkg", "Pkg"),
    ("Pk", "Pk"),
    ("LevelBlob", "LevelBlob"),
    ("SceneNode", "Scene"),
    ("Texture", "Texture"),
)


def _domain_for(name: str) -> str:
    for prefix, domain in _DOMAIN_PREFIXES:
        if name.startswith(prefix):
            return domain
    if "_" in name:
        return name.split("_", 1)[0]
    for domain in sorted(_DOMAIN_USE, key=len, reverse=True):
        if name.startswith(domain):
            return domain
    return "PCDOGS"


def _words(name: str) -> str:
    spaced = name.replace("_", " ")
    spaced = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", spaced)
    return " ".join(spaced.split())


def _kind_sentence(name: str) -> str:
    for suffix, sentence in _KIND_USE.items():
        if name.endswith(suffix) or f"_{suffix}" in name:
            return sentence
    return "Keeps the field layout that subsystem callers read or write directly."


def struct_doc(name: str) -> str:
    """Return a concise, usage-oriented description for a generated structure."""

    if name in _SPECIFIC_USE:
        return _SPECIFIC_USE[name]
    domain = _domain_for(name)
    if domain == "PCDOGS":
        return f"Used by game code. {_kind_sentence(name)}"
    return f"Used in {_DOMAIN_USE[domain]}. {_kind_sentence(name)}"


def install_struct_docs(blueprint: Any) -> None:
    """Patch a blueprint instance so structs get usage docs unless explicitly overridden."""

    original: Callable[..., Any] = blueprint.struct

    def documented_struct(name: str, *members: Any, **kwargs: Any) -> Any:
        kwargs.setdefault("doc", struct_doc(name))
        return original(name, *members, **kwargs)

    blueprint.struct = documented_struct
