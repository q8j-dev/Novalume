#!/usr/bin/env python3

"""Build a Roblox XML model from the supplied Studio PlayerModule sources.

The Studio distribution contains the authoritative sources as a filesystem
project rather than as a runtime model.  This converter preserves every source
byte as XML text and applies the same filename conventions described by the
adjacent default.rbxp project (init.lua, *.module.lua, *.client.lua, and
*.server.lua).
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET


def source_class(path: Path, default: str = "ModuleScript") -> str:
    if path.name.endswith(".client.lua"):
        return "LocalScript"
    if path.name.endswith(".server.lua"):
        return "Script"
    return default


def instance_name(path: Path) -> str:
    name = path.name
    for suffix in (".client.lua", ".server.lua", ".module.lua", ".lua"):
        if name.endswith(suffix):
            return name[: -len(suffix)]
    return name


class ModelBuilder:
    def __init__(self) -> None:
        self.next_referent = 1
        self.sources: list[dict[str, str]] = []

    def item(self, class_name: str, name: str, source: Path | None = None) -> ET.Element:
        element = ET.Element(
            "Item",
            {"class": class_name, "referent": f"RBX_CURRENT_PLAYER_{self.next_referent:04d}"},
        )
        self.next_referent += 1
        properties = ET.SubElement(element, "Properties")
        ET.SubElement(properties, "string", {"name": "Name"}).text = name
        if source is not None:
            raw = source.read_bytes()
            text = raw.decode("utf-8")
            ET.SubElement(properties, "ProtectedString", {"name": "Source"}).text = text
            ET.SubElement(properties, "Content", {"name": "LinkedSource"}).append(
                ET.Element("null")
            )
            if class_name in ("LocalScript", "Script"):
                ET.SubElement(properties, "bool", {"name": "Disabled"}).text = "false"
            self.sources.append(
                {
                    "path": source.as_posix(),
                    "sha256": hashlib.sha256(raw).hexdigest(),
                }
            )
        return element

    def populate_directory(self, parent: ET.Element, directory: Path) -> None:
        files = {path.name: path for path in directory.iterdir() if path.is_file()}
        directories = sorted(path for path in directory.iterdir() if path.is_dir())
        consumed: set[str] = {"init.lua", ".robloxrc"}

        for child_dir in directories:
            init_source = child_dir / "init.lua"
            sibling_source = files.get(f"{child_dir.name}.lua")
            if init_source.is_file():
                child = self.item("ModuleScript", child_dir.name, init_source)
            elif sibling_source is not None:
                child = self.item(source_class(sibling_source), child_dir.name, sibling_source)
                consumed.add(sibling_source.name)
            else:
                child = self.item("Folder", child_dir.name)
            self.populate_directory(child, child_dir)
            parent.append(child)

        for source in sorted(files.values()):
            if source.name in consumed or source.suffix != ".lua":
                continue
            parent.append(self.item(source_class(source), instance_name(source), source))


def build(studio_root: Path, output: Path, manifest: Path) -> None:
    scripts = studio_root / "ExtraContent/scripts/PlayerScripts"
    loader = scripts / "StarterPlayerScripts/PlayerScriptsLoader.lua"
    module_source = scripts / "StarterPlayerScripts/PlayerModule.module.lua"
    module_children = scripts / "StarterPlayerScripts/PlayerModule.module"
    required = (loader, module_source, module_children)
    if not all(path.exists() for path in required):
        raise SystemExit("supplied Studio build does not contain the current PlayerModule project")

    builder = ModelBuilder()
    root = ET.Element(
        "roblox",
        {
            "xmlns:xmime": "http://www.w3.org/2005/05/xmlmime",
            "xmlns:xsi": "http://www.w3.org/2001/XMLSchema-instance",
            "xsi:noNamespaceSchemaLocation": "http://www.roblox.com/roblox.xsd",
            "version": "4",
        },
    )
    ET.SubElement(root, "External").text = "null"
    ET.SubElement(root, "External").text = "nil"

    root.append(builder.item("LocalScript", "PlayerScriptsLoader", loader))
    player_module = builder.item("ModuleScript", "PlayerModule", module_source)
    builder.populate_directory(player_module, module_children)
    root.append(player_module)

    output.parent.mkdir(parents=True, exist_ok=True)
    ET.indent(root, space="  ")
    # The preserved XML reader treats a self-closing tag name as `null /`.
    # Emit explicit open/close elements, matching Roblox-authored RBXMX files.
    ET.ElementTree(root).write(
        output, encoding="utf-8", xml_declaration=True, short_empty_elements=False
    )
    payload = {
        "format": 1,
        "studio_root": studio_root.as_posix(),
        "source_count": len(builder.sources),
        "sources": sorted(builder.sources, key=lambda entry: entry["path"]),
        "model_sha256": hashlib.sha256(output.read_bytes()).hexdigest(),
    }
    manifest.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--studio-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    args = parser.parse_args()
    build(args.studio_root.resolve(), args.output.resolve(), args.manifest.resolve())


if __name__ == "__main__":
    main()
