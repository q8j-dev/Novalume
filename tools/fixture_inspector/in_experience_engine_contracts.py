#!/usr/bin/env python3
"""Inventory engine instance/property contracts used by a Player UI model.

This reads only binary model chunk headers. It records which concrete engine
classes and serialized properties the supplied in-experience model contains,
providing an auditable checklist for native feature backports.
"""

from __future__ import annotations

import argparse
import collections
import json
import pathlib
import re

from in_experience_contracts import FormatError, model_chunks, read_string, read_u32


UI_CLASSES = {
    "BillboardGui",
    "CanvasGroup",
    "Frame",
    "ImageButton",
    "ImageLabel",
    "ScreenGui",
    "ScrollingFrame",
    "TextBox",
    "TextButton",
    "TextLabel",
    "UIAspectRatioConstraint",
    "UICorner",
    "UIFlexItem",
    "UIGradient",
    "UIGridLayout",
    "UIListLayout",
    "UIPadding",
    "UIScale",
    "UISizeConstraint",
    "UIStroke",
    "UITableLayout",
    "UITextSizeConstraint",
    "UIDragDetector",
    "UIPageLayout",
    "VideoFrame",
    "ViewportFrame",
}


CREATE_PATTERN = re.compile(
    r'(?:createElement\d*|create\d*|Instance\.new)\(\s*["\']([A-Za-z][A-Za-z0-9_]*)["\']'
)
CREATE_TABLE_PATTERN = re.compile(
    r'(?:[A-Za-z_][A-Za-z0-9_.]*\.)?createElement\d*\(\s*'
    r'["\']([A-Za-z][A-Za-z0-9_]*)["\']\s*,\s*\{'
)
INSTANCE_VARIABLE_PATTERN = re.compile(
    r'(?:local\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*=\s*Instance\.new\(\s*'
    r'["\']([A-Za-z][A-Za-z0-9_]*)["\']\s*\)'
)

REGISTERED_CLASS_PATTERN = re.compile(r"RBX_REGISTER_CLASS\(\s*([A-Za-z][A-Za-z0-9_]*)\s*\)")
RUNTIME_CLASS_ALIASES = {
    "GuiImageButton": "ImageButton",
    "GuiTextButton": "TextButton",
}


def _table_literal(source: str, opening_brace: int) -> str:
    depth = 0
    quote: str | None = None
    index = opening_brace
    while index < len(source):
        char = source[index]
        if quote:
            if char == "\\":
                index += 2
                continue
            if char == quote:
                quote = None
        elif char in {'"', "'"}:
            quote = char
        elif source.startswith("--[[", index):
            end = source.find("]]", index + 4)
            index = len(source) if end < 0 else end + 2
            continue
        elif source.startswith("--", index):
            end = source.find("\n", index + 2)
            index = len(source) if end < 0 else end + 1
            continue
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[opening_brace + 1 : index]
        index += 1
    return ""


def _top_level_property_keys(table: str) -> list[str]:
    keys: list[str] = []
    curly = paren = bracket = 0
    quote: str | None = None
    index = 0
    entry_start = True
    while index < len(table):
        char = table[index]
        if quote:
            if char == "\\":
                index += 2
                continue
            if char == quote:
                quote = None
            index += 1
            continue
        if char in {'"', "'"}:
            quote = char
            index += 1
            continue
        if table.startswith("--[[", index):
            end = table.find("]]", index + 4)
            index = len(table) if end < 0 else end + 2
            continue
        if table.startswith("--", index):
            end = table.find("\n", index + 2)
            index = len(table) if end < 0 else end + 1
            continue
        if char == "{":
            curly += 1
        elif char == "}":
            curly -= 1
        elif char == "(":
            paren += 1
        elif char == ")":
            paren -= 1
        elif char == "[":
            bracket += 1
        elif char == "]":
            bracket -= 1
        elif curly == paren == bracket == 0:
            if char == ",":
                entry_start = True
            elif entry_start and (char.isalpha() or char == "_"):
                match = re.match(r"[A-Za-z_][A-Za-z0-9_]*", table[index:])
                assert match
                key = match.group(0)
                cursor = index + len(key)
                while cursor < len(table) and table[cursor].isspace():
                    cursor += 1
                if cursor < len(table) and table[cursor] == "=":
                    keys.append(key)
                entry_start = False
                index = cursor
                continue
            elif not char.isspace():
                entry_start = False
        index += 1
    return keys


def source_property_contracts(source: str) -> dict[str, collections.Counter[str]]:
    result: dict[str, collections.Counter[str]] = collections.defaultdict(collections.Counter)
    for match in CREATE_TABLE_PATTERN.finditer(source):
        table = _table_literal(source, match.end() - 1)
        result[match.group(1)].update(_top_level_property_keys(table))
    for match in INSTANCE_VARIABLE_PATTERN.finditer(source):
        variable, class_name = match.groups()
        assignment = re.compile(rf"\b{re.escape(variable)}\.([A-Za-z_][A-Za-z0-9_]*)\s*=")
        result[class_name].update(assignment.findall(source[match.end() :]))
    return result


def inspect_model(
    path: pathlib.Path,
    decompiled: pathlib.Path | None = None,
    source_root: pathlib.Path | None = None,
) -> dict[str, object]:
    if decompiled and not decompiled.is_dir():
        raise FormatError(f"decompiled module directory does not exist: {decompiled}")
    data = path.read_bytes()
    chunks = model_chunks(data)
    classes: dict[int, tuple[str, int]] = {}
    property_names: dict[str, set[str]] = collections.defaultdict(set)

    for tag, chunk in chunks:
        if tag != b"INST":
            continue
        class_id, offset = read_u32(chunk, 0)
        class_name, offset = read_string(chunk, offset)
        if offset >= len(chunk):
            raise FormatError("truncated INST chunk")
        offset += 1
        count, _ = read_u32(chunk, offset)
        classes[class_id] = (class_name, count)

    for tag, chunk in chunks:
        if tag != b"PROP":
            continue
        class_id, offset = read_u32(chunk, 0)
        property_name, offset = read_string(chunk, offset)
        if class_id not in classes or offset >= len(chunk):
            raise FormatError("property references an unknown class")
        property_names[classes[class_id][0]].add(property_name)

    source_counts: collections.Counter[str] = collections.Counter()
    source_properties: dict[str, collections.Counter[str]] = collections.defaultdict(collections.Counter)
    decompiled_file_count = 0
    if decompiled:
        for source in decompiled.rglob("*.luau"):
            decompiled_file_count += 1
            text = source.read_text(encoding="utf-8", errors="ignore")
            source_counts.update(CREATE_PATTERN.findall(text))
            for class_name, properties in source_property_contracts(text).items():
                source_properties[class_name].update(properties)
        if decompiled_file_count == 0:
            raise FormatError(f"decompiled module directory contains no .luau files: {decompiled}")

    registered_classes: set[str] = set()
    if source_root:
        registration = source_root / "engine/datamodel/runtime/src/factoryregistration.cpp"
        cpp_classes = REGISTERED_CLASS_PATTERN.findall(registration.read_text(encoding="utf-8"))
        registered_classes.update(RUNTIME_CLASS_ALIASES.get(name, name) for name in cpp_classes)

    ui = []
    all_names = {name for name, _ in classes.values()} | set(source_counts)
    ids_by_name = {name: class_id for class_id, (name, _) in classes.items()}
    counts_by_name = {name: count for name, count in classes.values()}
    for class_name in sorted(all_names):
        if class_name in UI_CLASSES or class_name.startswith("UI"):
            ui.append(
                {
                    "class": class_name,
                    "classId": ids_by_name.get(class_name),
                    "serializedInstances": counts_by_name.get(class_name, 0),
                    "sourceCreations": source_counts[class_name],
                    "sourceProperties": sorted(source_properties[class_name]),
                    "sourcePropertyUses": dict(sorted(source_properties[class_name].items())),
                    "properties": sorted(property_names[class_name]),
                    "registeredNativeClass": class_name in registered_classes if source_root else None,
                }
            )

    return {
        "schema": 1,
        "model": str(path.resolve()),
        "decompiledModuleFileCount": decompiled_file_count,
        "allClassCount": len(classes),
        "uiClassCount": len(ui),
        "uiSerializedInstanceCount": sum(item["serializedInstances"] for item in ui),
        "uiSourceCreationCount": sum(item["sourceCreations"] for item in ui),
        "uiClasses": ui,
        "unregisteredNativeClasses": sorted(
            item["class"] for item in ui if source_root and not item["registeredNativeClass"]
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("model", type=pathlib.Path)
    parser.add_argument("--decompiled", type=pathlib.Path)
    parser.add_argument("--source-root", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    args = parser.parse_args()
    result = inspect_model(args.model, args.decompiled, args.source_root)
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded, encoding="utf-8")
    else:
        print(encoded, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
